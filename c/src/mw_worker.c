#include <signal.h>
#include <stdint.h>
#include <sys/epoll.h>
#include "lua_bind.h"
#include "lua_utils.h"
#include "mw_worker.h"
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

#define th_remove_res_from_epfd(_th_, _epfd_) do {\
  if (asthread_th_is_simple_mode(_th_)) {\
    asthread_remove_res_from_epfd(_th_, _epfd_);\
  }\
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
th_yield_for_io(as_thread_t *th, as_mw_worker_ctx_t *ctx) {
  lua_State *T = th->T;

  as_thread_res_t *res = (as_thread_res_t *)lua_touserdata(T, -3);
  int io_type = lua_tointeger(T, -2);
  int secs = lua_tointeger(T, -1);
  lua_pop(T, 4);

  secs = secs < 0 ? ctx->conn_timeout : secs;
  asthread_th_to_iowait(th, time(NULL) + secs, &ctx->io_pool);

  uint32_t events = io_type == LAS_S_WAIT_FOR_INPUT ?
      EPOLLIN | EPOLLET :
      EPOLLOUT | EPOLLET;
  asthread_res_ev_add(res, ctx->epfd, events);
}


static inline void
th_yield_for_sleep(as_thread_t *th, as_mw_worker_ctx_t *ctx) {
  lua_State *T = th->T;

  int secs = lua_tointeger(T, -1);
  lua_pop(T, 2);

  asthread_th_to_sleep(th, time(NULL) + secs, &ctx->sleep_pool);
}


static inline void
th_yield_for_ev(as_thread_t *th, as_mw_worker_ctx_t *ctx) {
  lua_State *T = th->T;

  int secs = lua_tointeger(T, -1);
  lua_pop(T, 2);

  asthread_th_to_sleep(th, time(NULL) + secs, &ctx->sleep_pool);
}


static void
thread_resume(as_thread_t *th, as_mw_worker_ctx_t *ctx, int nargs) {
  if (asthread_th_is_stop(th)) {
    return;
  }

  lua_State *T = th->T;
  // debug_log("r: %d - %p\n", th->tid, T);
  int n = lua_gettop(T) - nargs;

  if (asthread_th_is_ready(th)) {
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

    } else if (n_res == 2 && lua_isinteger(T, -2) &&
               lua_tointeger(T, -2) == LAS_S_YIELD_FOR_EV) {

      th_yield_for_ev(th, ctx);
      return;

    }
  } else if (ret != LUA_OK) {
    lb_pop_error_msg(T);
  }

  asthread_th_to_stop(th, ctx->stop_threads);

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
    // set_socket_send_buffer_size(fd, 2048);

    as_thread_t *th = mpf_alloc(ctx->mem_pool, sizeof(as_thread_t));
    as_tid_t tid = asthread_th_init(th, ctx->L);
    if (tid == -1) {
      mpf_recycle(th);
      continue;
    }

    as_thread_res_t *res = mpf_alloc(ctx->mem_pool, sizeof_thread_res(int));
    *(int *)res->d = fd;
    asthread_res_init(res, res_free_f, res_fd_f);
    asthread_res_add_to_th(res, th);
    asthread_res_ev_init(res, ctx->epfd);

    th->mfd_res = res;

    lua_State *T = th->T;
    lbind_set_thread_local_vars(T, 1, LLK_THREAD, LTYPE_PTR, th);

    thread_resume(th, ctx, 0);
  }
}


static void
handle_fd_read(as_mw_worker_ctx_t *ctx, as_thread_res_t *res) {
  if (asthread_th_is_stop(res->th)) {
    return;
  }
  asthread_th_del_from_pool(res->th);
  th_remove_res_from_epfd(res->th, ctx->epfd);

  lua_State *T = res->th->T;
  lua_pushinteger(T, LAS_S_RESUME_IO);
  lua_pushlightuserdata(T, res);
  lua_pushinteger(T, LAS_S_READY_TO_INPUT);
  thread_resume(res->th, ctx, 3);
}


static void
handle_fd_write(as_mw_worker_ctx_t *ctx, as_thread_res_t *res) {
  if (asthread_th_is_stop(res->th)) {
    return;
  }
  asthread_th_del_from_pool(res->th);
  th_remove_res_from_epfd(res->th, ctx->epfd);

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
    if (asthread_th_is_stop(res->th)) {
      return;
    }
    asthread_th_del_from_pool(res->th);
    th_remove_res_from_epfd(res->th, ctx->epfd);

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

    asthread_th_free(th, NULL);
    mpf_recycle(th->mfd_res);
    mpf_recycle(th);
  }
}


static inline void
p_io_timeout_thread(as_rb_node_t *n, as_mw_worker_ctx_t *ctx) {
  as_thread_t *th = rb_node_to_thread(n);
  if (asthread_th_is_stop(th)) {
    return;
  }
  th_remove_res_from_epfd(th, ctx->epfd);

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
  if (asthread_th_is_stop(th)) {
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


static inline void
r_thread_from_pool(as_rb_node_t *n) {
  as_thread_t *th = rb_node_to_thread(n);

  asthread_th_free(th, NULL);
  mpf_recycle(th->mfd_res);
  mpf_recycle(th);
}


static void
recycle_threads_from_pool(as_mw_worker_ctx_t *ctx) {
  rb_tree_postorder_travel(ctx->io_pool.root, r_thread_from_pool);
  rb_tree_postorder_travel(ctx->sleep_pool.root, r_thread_from_pool);
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
  lbind_reg_values(ctx.L, 1, LRK_WORKER_CTX, LTYPE_PTR, &ctx);
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

  process_stop_threads(&ctx);
  recycle_threads_from_pool(&ctx);

  mpf_recycle(ctx.stop_threads);
  mpf_recycle(ctx.cfd_res);
  lua_close(ctx.L);
  mpf_destroy(ctx.mem_pool);
}


void
worker_process(int channel_fd, as_lua_pconf_t *cnf) {
  process(channel_fd, cnf, 0);
}


void
test_worker_process(int fd, as_lua_pconf_t *cnf) {
  process(fd, cnf, 1);
}
