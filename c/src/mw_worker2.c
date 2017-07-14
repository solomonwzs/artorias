#include "mw_worker.h"
#include <signal.h>
#include <stdint.h>
#include <sys/epoll.h>
#include "lua_bind.h"
#include "lua_utils.h"
#include "rb_tree.h"
#include "server.h"

#define _MAX_EVENTS   100

#define get_cnf_int_val(_cnf_, _n_, ...) ({\
  as_cnf_return_t __ret = lpconf_get_pconf_value(_cnf_, _n_, ## __VA_ARGS__);\
  __ret.val.i;\
})

#define get_cnf_str_val(_cnf_, _n_, ...) ({\
  as_cnf_return_t __ret = lpconf_get_pconf_value(_cnf_, _n_, ## __VA_ARGS__);\
  __ret.val.s;\
})

#define extract_thread(_th_, _ctx_) do {\
  if ((_th_)->status == AS_TSTATUS_STOP) {\
    return;\
  }\
  asthread_pool_delete(_th_);\
  remove_th_res_from_epfd(_th_, _ctx_);\
} while(0)


static volatile int keep_running = 1;
static void
init_handler(int dummy) {
  keep_running = 0;
}


static int
res_free_f(as_thread_res_t *res, void *f_ptr) {
  int *fd = (int *)res->d;
  close(*fd);
  return 0;
}


static int
res_fd_f(as_thread_res_t *res) {
  return *(int *)res->d;
}


static inline void
remove_th_res_from_epfd(as_thread_t *th, as_mw_worker_ctx_t *ctx) {
  as_dlist_node_t *dn = th->res_head;
  while (dn != NULL) {
    as_thread_res_t *res = dl_node_to_res(dn);

    if (res->status == AS_RSTATUS_EPFD) {
      res->status = AS_RSTATUS_IDLE;
      epoll_ctl(ctx->epfd, EPOLL_CTL_DEL, res->fdf(res), NULL);
    }

    dn = dn->next;
  }
}


static inline void
th_yield_for_io(as_thread_t *th, as_mw_worker_ctx_t *ctx) {
  lua_State *T = th->T;

  as_thread_res_t *res = (as_thread_res_t *)lua_touserdata(T, -3);
  int io_type = lua_tointeger(T, -2);
  int secs = lua_tointeger(T, -1);
  lua_pop(T, 4);

  secs = secs < 0 ? ctx->conn_timeout : secs;
  th->et = time(NULL) + secs;
  th->status = AS_TSTATUS_RUN;

  th->pool = &ctx->io_pool;
  asthread_pool_insert(th);

  struct epoll_event event;
  event.data.ptr = res;
  event.events = io_type == LAS_S_WAIT_FOR_INPUT ?
      EPOLLIN | EPOLLET :
      EPOLLOUT | EPOLLET;
  if (epoll_ctl(ctx->epfd, EPOLL_CTL_ADD, res->fdf(res), &event) == 0) {
    res->status = AS_RSTATUS_EPFD;
  } else {
    debug_perror("epoll_ctr");
  }
}


static inline void
th_yield_for_sleep(as_thread_t *th, as_mw_worker_ctx_t *ctx) {
  lua_State *T = th->T;

  int secs = lua_tointeger(T, -1);
  lua_pop(T, 2);

  th->et = time(NULL) + secs;
  th->status = AS_TSTATUS_SLEEP;

  th->pool = &ctx->sleep_pool;
  asthread_pool_insert(th);
}


static void
thread_resume(as_thread_t *th, as_mw_worker_ctx_t *ctx, int nargs) {
  if (th->status == AS_TSTATUS_STOP) {
    return;
  }

  lua_State *T = th->T;
  // debug_log("r: %d - %p\n", th->tid, T);
  int n = lua_gettop(T) - nargs;

  if (th->status == AS_TSTATUS_READY) {
    lbind_get_lcode_chunk(T, ctx->lfile);
  }
  int ret = alua_resume(T, nargs);

  if (ret == LUA_YIELD) {
    int n_res = lua_gettop(T) - n;
    if (n_res == 4 && lua_isinteger(T, -4) &&
        lua_tointeger(T, -4) == LAS_S_YIELD_FOR_IO) {

      th_yield_for_io(th, ctx);
      return;

    } else if (n_res == 2 && lua_isinteger(T, -2) &&
               lua_tointeger(T, -2) == LAS_S_YIELD_FOR_SLEEP) {

      th_yield_for_sleep(th, ctx);
      return;

    }
  } else if (ret != LUA_OK) {
    lb_pop_error_msg(T);
  }

  th->status = AS_TSTATUS_STOP;
  th->pool = NULL;
  asthread_array_add(ctx->stop_threads, th);

  return;
}


static void
handle_accept(as_mw_worker_ctx_t *ctx, int single_mode) {
  int (*new_fd_func)(int);
  if (single_mode) {
    new_fd_func = new_accept_fd;
  } else {
    new_fd_func = recv_fd_from_socket;
  }

  while (1) {
    int fd = new_fd_func(ctx->cfd);
    if (fd == -1) {
      break;
    }
    set_non_block(fd);

    as_thread_t *th = mpf_alloc(ctx->mem_pool, sizeof(as_thread_t));
    as_tid_t tid = asthread_init(th, ctx->L);
    if (tid == -1) {
      mpf_recycle(th);
      continue;
    }

    as_thread_res_t *res = mpf_alloc(ctx->mem_pool, sizeof_thread_res(int));
    asthread_res_init(res, res_free_f, res_fd_f);
    asthread_res_add_to_th(res, th);
    *(int *)res->d = fd;

    th->mfd_res = res;

    lua_State *T = th->T;
    lbind_set_thread_local_var_ptr(T, "th", th);

    thread_resume(th, ctx, 0);
  }
}


static void
handle_fd_read(as_mw_worker_ctx_t *ctx, as_thread_res_t *res) {
  if (res->th->status == AS_TSTATUS_STOP) {
    return;
  }
  asthread_pool_delete(res->th);
  remove_th_res_from_epfd(res->th, ctx);

  lua_State *T = res->th->T;
  lua_pushinteger(T, LAS_S_RESUME_IO);
  lua_pushlightuserdata(T, res);
  lua_pushinteger(T, LAS_S_READY_TO_INPUT);
  thread_resume(res->th, ctx, 3);
}


static void
handle_fd_write(as_mw_worker_ctx_t *ctx, as_thread_res_t *res) {
  if (res->th->status == AS_TSTATUS_STOP) {
    return;
  }
  asthread_pool_delete(res->th);
  remove_th_res_from_epfd(res->th, ctx);

  lua_State *T = res->th->T;
  lua_pushinteger(T, LAS_S_RESUME_IO);
  lua_pushlightuserdata(T, res);
  lua_pushinteger(T, LAS_S_READY_TO_OUTPUT);
  thread_resume(res->th, ctx, 3);
}


static void
handle_fd_error(as_mw_worker_ctx_t *ctx, as_thread_res_t *res) {
  if (res == ctx->cfd_res) {
    int fd = *((int *)ctx->cfd_res->d);
    close(fd);

  } else {
    if (res->th->status == AS_TSTATUS_STOP) {
      return;
    }
    asthread_pool_delete(res->th);
    remove_th_res_from_epfd(res->th, ctx);

    lua_State *T = res->th->T;
    lua_pushinteger(T, LAS_S_RESUME_IO_ERROR);
    lua_pushlightuserdata(T, res);
    lua_pushinteger(T, errno);
    thread_resume(res->th, ctx, 3);
  }
}


static void
process_stop_threads(as_mw_worker_ctx_t *ctx) {
  for (int i = 0; i < ctx->stop_threads->n; ++i) {
    as_thread_t *th = ctx->stop_threads->ths[i];
    as_thread_res_t *mfd_res = th->mfd_res;

    asthread_free(th, NULL);
    mpf_recycle(th);
    mpf_recycle(mfd_res);
  }
}


static inline void
p_io_timeout_thread(as_rb_node_t *n, as_mw_worker_ctx_t *ctx) {
  as_thread_t *th = rb_node_to_thread(n);
  if (th->status == AS_TSTATUS_STOP) {
    return;
  }
  remove_th_res_from_epfd(th, ctx);

  lua_State *T = th->T;
  lua_pushinteger(T, LAS_S_RESUME_IO_TIMEOUT);
  thread_resume(th, ctx, 1);
}


static void
process_io_timeout(as_mw_worker_ctx_t *ctx) {
  as_rb_node_t *timeout_tr = asthread_remove_timeout_threads(&ctx->io_pool);
  rb_tree_postorder_travel(timeout_tr, p_io_timeout_thread, ctx);
}


static void
process_io(as_mw_worker_ctx_t *ctx, int single_mode) {
  struct epoll_event events[_MAX_EVENTS];
  int active_cnt = epoll_wait(ctx->epfd, events, _MAX_EVENTS, 500);

  for (int i = 0; i < active_cnt; ++i) {
    as_thread_res_t *res = (as_thread_res_t *)events[i].data.ptr;

    if (events[i].events & EPOLLERR || events[i].events & EPOLLHUP ||
        !(events[i].events & EPOLLIN || events[i].events & EPOLLOUT)) {
      handle_fd_error(ctx, res);

    } else if (res == ctx->cfd_res) {
      handle_accept(ctx, single_mode);

    } else if (events[i].events & EPOLLIN) {
      handle_fd_read(ctx, res);

    } else if (events[i].events & EPOLLOUT) {
      handle_fd_write(ctx, res);

    }
  }
}


static inline void
p_sleep_thread(as_rb_node_t *n, as_mw_worker_ctx_t *ctx) {
  as_thread_t *th = rb_node_to_thread(n);
  if (th->status == AS_TSTATUS_STOP) {
    return;
  }

  lua_State *T = th->T;
  lua_pushinteger(T, LAS_S_RESUME_SLEEP);
  thread_resume(th, ctx, 1);
}


static void
process_sleep(as_mw_worker_ctx_t *ctx) {
  as_rb_node_t *timeout_tr = asthread_remove_timeout_threads(
      &ctx->sleep_pool);
  rb_tree_postorder_travel(timeout_tr, p_sleep_thread, ctx);
}


static void
process(int cfd, as_lua_pconf_t *cnf, int single_mode) {
  set_non_block(cfd);
  if (single_mode) {
    signal(SIGINT, init_handler);
  }

  as_mw_worker_ctx_t ctx;
  ctx.cfd = cfd;
  ctx.io_pool.root = NULL;
  ctx.sleep_pool.root = NULL;

  size_t fs[] = {8, 12, 16, 24, 32, 48, 64, 128, 256, 384, 512, 768, 1064};
  ctx.mem_pool = mpf_new(fs, sizeof(fs) / sizeof(fs[0]));

  ctx.conn_timeout = get_cnf_int_val(cnf, 1, "conn_timeout");
  ctx.lfile = get_cnf_str_val(cnf, 1, "worker");

  ctx.cfd_res = mpf_alloc(ctx.mem_pool, sizeof_thread_res(int));
  ctx.cfd_res->freef = NULL;
  ctx.cfd_res->th = NULL;
  *((int *)ctx.cfd_res->d) = cfd;

  ctx.epfd = epoll_create(1);

  struct epoll_event event;
  event.data.ptr = ctx.cfd_res;
  event.events = EPOLLIN | EPOLLET;
  epoll_ctl(ctx.epfd, EPOLL_CTL_ADD, cfd, &event);

  ctx.L = lbind_new_state(ctx.mem_pool);
  lbind_init_state(ctx.L);
  lbind_append_lua_cpath(ctx.L, get_cnf_str_val(cnf, 1, "lua_cpath"));
  lbind_append_lua_path(ctx.L, get_cnf_str_val(cnf, 1, "lua_path"));
  lbind_reg_value_ptr(ctx.L, LRK_WORKER_CTX, &ctx);
  lbind_ref_lcode_chunk(ctx.L, ctx.lfile);

  ctx.stop_threads = mpf_alloc(ctx.mem_pool, sizeof_thread_array(_MAX_EVENTS));
  while (keep_running) {
    ctx.stop_threads->n = 0;

    process_io(&ctx, single_mode);
    process_io_timeout(&ctx);
    process_sleep(&ctx);
    process_stop_threads(&ctx);
    // lua_gc(ctx.L, LUA_GCCOLLECT, 0);
  }

  mpf_recycle(ctx.stop_threads);
  mpf_recycle(ctx.cfd_res);
  lua_close(ctx.L);
  mpf_destroy(ctx.mem_pool);
}


void
worker_process2(int channel_fd, as_lua_pconf_t *cnf) {
  process(channel_fd, cnf, 0);
}


void
test_worker_process2(int fd, as_lua_pconf_t *cnf) {
  process(fd, cnf, 1);
}
