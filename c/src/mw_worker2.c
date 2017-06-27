#include "mw_worker.h"
#include "mem_pool.h"
#include <signal.h>
#include <stdint.h>
#include <sys/epoll.h>
#include "lua_bind.h"
#include "lua_utils.h"
#include "rb_tree.h"
#include "server.h"
#include "thread.h"

#define _MAX_EVENTS           100

#define get_cnf_int_val(_cnf_, _n_, ...) ({\
  as_cnf_return_t __ret = lpconf_get_pconf_value(_cnf_, _n_, ## __VA_ARGS__);\
  __ret.val.i;\
})

#define get_cnf_str_val(_cnf_, _n_, ...) ({\
  as_cnf_return_t __ret = lpconf_get_pconf_value(_cnf_, _n_, ## __VA_ARGS__);\
  __ret.val.s;\
})

#define extract_thread(_ctx_, _res_) do {\
  if ((_res_)->th->status == AS_TSTATUS_STOP) {\
    return;\
  }\
  asthread_pool_delete((_res_)->th);\
  epoll_ctl(ctx->epfd, EPOLL_CTL_DEL, *((int *)(_res_)->d), NULL);\
} while(0)


typedef struct {
  as_mem_pool_fixed_t   *mem_pool;
  as_rb_tree_t          io_pool;
  as_rb_tree_t          sleep_pool;
  as_thread_array_t     *stop_threads;
  as_thread_res_t       *cfd_res;
  lua_State             *L;
  int                   epfd;
  int                   cfd;
  int                   conn_timeout;
  const char            *lfile;
} _as_mw_worker_ctx_t;


static int close_fd(void *ptr) {
  int *fd = (int *)ptr;
  close(*fd);
  return 0;
}


static volatile int keep_running = 1;
static void
init_handler(int dummy) {
  keep_running = 0;
}


static void
thread_resume(as_thread_t *th, _as_mw_worker_ctx_t *ctx, int nargs) {
  if (th->status == AS_TSTATUS_STOP) {
    return;
  }

  lua_State *T = th->T;
  int n = lua_gettop(T) - nargs;

  if (th->status == AS_TSTATUS_READY) {
    lbind_get_lcode_chunk(T, ctx->lfile);
  }
  int ret = alua_resume(T, nargs);

  if (ret == LUA_YIELD) {
    int n_res = lua_gettop(T) - n;
    if (n_res == 4 && lua_isinteger(T, -4) &&
        lua_tointeger(T, -4) == LAS_YIELD_FOR_IO) {

      as_thread_res_t *res = (as_thread_res_t *)lua_touserdata(T, -3);
      int io_type = lua_tointeger(T, -2);
      int secs = lua_tointeger(T, -1);

      secs = secs < 0 ? ctx->conn_timeout : secs;
      th->et = time(NULL) + secs;
      th->status = AS_TSTATUS_RUN;

      th->pool = &ctx->io_pool;
      asthread_pool_insert(th);

      struct epoll_event event;
      event.data.ptr = res;
      event.events = io_type == LAS_WAIT_FOR_INPUT ?
          EPOLLIN | EPOLLET :
          EPOLLOUT | EPOLLET;
      epoll_ctl(ctx->epfd, EPOLL_CTL_ADD, 0, &event);

      return;

    } else if (n_res == 2 && lua_isinteger(T, -2) &&
               lua_tointeger(T, -2) == LAS_YIELD_FOR_SLEEP) {

      int secs = lua_tointeger(T, -1);

      th->et = time(NULL) + secs;
      th->status = AS_TSTATUS_SLEEP;

      th->pool = &ctx->sleep_pool;
      asthread_pool_insert(th);

      return;

    }
  }

  th->status = AS_TSTATUS_STOP;
  th->pool = NULL;
  asthread_array_add(ctx->stop_threads, th);

  return;
}


static void
handle_accept(_as_mw_worker_ctx_t *ctx, int single_mode) {
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
    res->resf = close_fd;
    res->th = th;
    *((int *)res->d) = fd;

    th->mfd_res = res;

    lua_State *T = th->T;
    lbind_set_thread_local_var_ptr(T, "mfd", res);

    thread_resume(th, ctx, 0);
  }
}


static void
handle_fd_read(_as_mw_worker_ctx_t *ctx, as_thread_res_t *res) {
  extract_thread(ctx, res);

  lua_State *T = res->th->T;
  lua_pushinteger(T, LAS_RESUME_IO);
  lua_pushlightuserdata(T, res);
  lua_pushinteger(T, LAS_READY_TO_INPUT);
  thread_resume(res->th, ctx, 3);
}


static void
handle_fd_write(_as_mw_worker_ctx_t *ctx, as_thread_res_t *res) {
  extract_thread(ctx, res);

  lua_State *T = res->th->T;
  lua_pushinteger(T, LAS_RESUME_IO);
  lua_pushlightuserdata(T, res);
  lua_pushinteger(T, LAS_READY_TO_OUTPUT);
  thread_resume(res->th, ctx, 3);
}


static void
handle_fd_error(_as_mw_worker_ctx_t *ctx, as_thread_res_t *res) {
  if (res == ctx->cfd_res) {
    int fd = *((int *)ctx->cfd_res->d);
    close(fd);

  } else {
    extract_thread(ctx, res);

    lua_State *T = res->th->T;
    lua_pushinteger(T, LAS_RESUME_IO_ERROR);
    lua_pushlightuserdata(T, res);
    lua_pushinteger(T, errno);
    thread_resume(res->th, ctx, 3);
  }
}


static void
process_stop_threads(_as_mw_worker_ctx_t *ctx) {
  for (int i = 0; ctx->stop_threads->n; ++i) {
    as_thread_t *th = ctx->stop_threads->ths[i];
    as_thread_res_t *mfd_res = th->mfd_res;

    asthread_free(th);
    mpf_recycle(th);
    mpf_recycle(mfd_res);
  }
}


static void inline
p_io_timeout_thread(as_rb_node_t *n, _as_mw_worker_ctx_t *ctx) {
  as_thread_t *th = rb_node_to_thread(n);
  if (th->status == AS_TSTATUS_STOP) {
    return;
  }

  lua_State *T = th->T;
}


static void
process_io_timeout_threads(_as_mw_worker_ctx_t *ctx) {
  as_rb_node_t *timeout_tr = asthread_remove_timeout_threads(&ctx->io_pool);
}


static void
process_io(_as_mw_worker_ctx_t *ctx, int single_mode) {
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


static void
process(int cfd, as_lua_pconf_t *cnf, int single_mode) {
  set_non_block(cfd);
  if (single_mode) {
    signal(SIGINT, init_handler);
  }

  _as_mw_worker_ctx_t ctx;
  ctx.cfd = cfd;
  ctx.io_pool.root = NULL;
  ctx.sleep_pool.root = NULL;

  size_t fs[] = {8, 12, 16, 24, 32, 48, 64, 128, 256, 384, 512, 768, 1064};
  ctx.mem_pool = mpf_new(fs, sizeof(fs) / sizeof(fs[0]));

  ctx.L = lbind_new_state(ctx.mem_pool);
  ctx.conn_timeout = get_cnf_int_val(cnf, 1, "conn_timeout");
  ctx.lfile = get_cnf_str_val(cnf, 1, "worker");

  ctx.cfd_res = mpf_alloc(ctx.mem_pool, sizeof_thread_res(int));
  ctx.cfd_res->resf = NULL;
  ctx.cfd_res->th = NULL;
  *((int *)ctx.cfd_res->d) = cfd;

  ctx.epfd = epoll_create(1);

  struct epoll_event event;
  event.data.ptr = ctx.cfd_res;
  event.events = EPOLLIN | EPOLLET;
  epoll_ctl(ctx.epfd, EPOLL_CTL_ADD, cfd, &event);

  lbind_init_state(ctx.L);
  lbind_append_lua_cpath(ctx.L, get_cnf_str_val(cnf, 1, "lua_cpath"));
  lbind_append_lua_path(ctx.L, get_cnf_str_val(cnf, 1, "lua_path"));
  lbind_reg_value_int(ctx.L, LRK_SERVER_EPFD, ctx.epfd);
  lbind_reg_value_ptr(ctx.L, LRK_MEM_POOL, ctx.mem_pool);
  lbind_ref_lcode_chunk(ctx.L, ctx.lfile);

  ctx.stop_threads = mpf_alloc(ctx.mem_pool, sizeof_thread_array(_MAX_EVENTS));
  while (keep_running) {
    ctx.stop_threads->n = 0;

    process_io(&ctx, single_mode);
    process_stop_threads(&ctx);
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
