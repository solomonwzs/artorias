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
#define _DEFAULT_TIMEOUT_SECS 5

#define get_cnf_int_val(_cnf_, _n_, ...) ({\
  as_cnf_return_t __ret = lpconf_get_pconf_value(_cnf_, _n_, ## __VA_ARGS__);\
  __ret.val.i;\
})

#define get_cnf_str_val(_cnf_, _n_, ...) ({\
  as_cnf_return_t __ret = lpconf_get_pconf_value(_cnf_, _n_, ## __VA_ARGS__);\
  __ret.val.s;\
})

#define p_mio(_pools_)    (_pools_)
#define p_io(_pools_)     (_pools_ + 1)
#define p_sleep(_pools_)  (_pools_ + 2)


static volatile int keep_running = 1;
static void
init_handler(int dummy) {
  keep_running = 0;
}


#define _AS_THS_TO_DONOTHING  0x00
#define _AS_THS_TO_MIO        0x01
#define _AS_THS_TO_IO         0x02
#define _AS_THS_TO_SLEEP      0x03
#define _AS_THS_TO_STOP       0x04
static void
thread_res_run(as_thread_t *th, const char *lfile,
               int *out_status, as_thread_res_t **out_res, int *out_io_type) {
  if (th->status == AS_TSTATUS_STOP) {
    *out_status = _AS_THS_TO_DONOTHING;
    return;
  }

  lua_State *T = th->T;
  int n = lua_gettop(T);

  if (th->status == AS_TSTATUS_READY) {
    lbind_get_lcode_chunk(T, lfile);
  }
  int ret = alua_resume(T, 0);
  if (ret == LUA_YIELD) {
    int n_res = lua_gettop(T) - n;
    if (n_res == 4 && lua_isinteger(T, -4) &&
        lua_tointeger(T, -4) == LAS_YIELD_FOR_IO) {

      as_thread_res_t *res = (as_thread_res_t *)lua_touserdata(T, -3);
      int io_type = lua_tointeger(T, -2);
      int secs = lua_tointeger(T, -1);

      secs = secs < 0 ? _DEFAULT_TIMEOUT_SECS : secs;
      th->et = time(NULL) + secs;
      th->status = AS_TSTATUS_RUN;

      *out_status = res == th->m_res ? _AS_THS_TO_MIO : _AS_THS_TO_IO;
      *out_res = res;
      *out_io_type = io_type;
      return;

    } else if (n_res == 2 && lua_isinteger(T, -2) &&
               lua_tointeger(T, -2) == LAS_YIELD_FOR_SLEEP) {

      int secs = lua_tointeger(T, -1);

      th->et = time(NULL) + secs;
      th->status = AS_TSTATUS_SLEEP;

      *out_status = _AS_THS_TO_SLEEP;
      return;

    }
  }

  th->status = _AS_THS_TO_STOP;

  *out_status = _AS_THS_TO_STOP;
  return;
}


static void
handler_accept(int cfd, as_mem_pool_fixed_t *mem_pool, lua_State *L,
               const char *lfile, as_rb_tree_t th_pools[],
               as_thread_array_t *stop_threads, int single_mode) {
  int (*new_fd_func)(int);
  if (single_mode) {
    new_fd_func = new_accept_fd;
  } else {
    new_fd_func = recv_fd_from_socket;
  }

  while (1) {
    int fd = new_fd_func(cfd);
    if (fd == -1) {
      break;
    }
    set_non_block(fd);

    as_thread_t *th = mpf_alloc(mem_pool, sizeof(as_thread_t));
    as_tid_t tid = asthread_init(th, L);
    if (tid == -1) {
      mpf_recycle(th);
      continue;
    }

    as_thread_res_t *res = mpf_alloc(mem_pool, sizeof_thread_res(int));
    res->type = AS_TRES_FD;
    res->th = th;
    *((int *)res->d) = fd;

    as_dlist_node_t *dln = &(res->node);
    dln->prev = NULL;
    dln->next = th->resl;
    th->resl = dln;

    th->m_res = res;

    lua_State *T = th->T;
    lbind_set_thread_local_var_ptr(T, "mfd", res);

    int out_status;
    as_thread_res_t *out_res;
    int out_io_type;
    thread_res_run(th, lfile, &out_status, &out_res, &out_io_type);

    if (out_status == _AS_THS_TO_MIO) {
      th->pool = p_mio(th_pools);
      asthread_pool_insert(th->pool, th);
    } else if (out_status == _AS_THS_TO_IO) {
      th->pool = p_io(th_pools);
      asthread_pool_insert(th->pool, th);
    } else if (out_status == _AS_THS_TO_SLEEP) {
      th->pool = p_sleep(th_pools);
      asthread_pool_insert(th->pool, th);
    } else if (out_status == _AS_THS_TO_STOP) {
      asthread_array_add(stop_threads, th);
    }
  }
}


static void
process_io(int epfd) {
  struct epoll_event events[_MAX_EVENTS];
  int active_cnt = epoll_wait(epfd, events, _MAX_EVENTS, 500);
  for (int i = 0; i < active_cnt; ++i) {
    as_thread_res_t *tres = (as_thread_res_t *)events[i].data.ptr;
    if (events[i].events & EPOLLERR || events[i].events & EPOLLHUP ||
        !(events[i].events & EPOLLIN || events[i].events & EPOLLOUT)) {
    } else if (tres->th == NULL) {
    }
  }
}


static void
process(int cfd, as_lua_pconf_t *cnf, int single_mode) {
  set_non_block(cfd);
  if (single_mode) {
    signal(SIGINT, init_handler);
  }

  size_t fs[] = {8, 12, 16, 24, 32, 48, 64, 128, 256, 384, 512, 768, 1064};
  as_mem_pool_fixed_t *mem_pool = mpf_new(fs, sizeof(fs) / sizeof(fs[0]));

  as_rb_tree_t pools[] = {{NULL}, {NULL}, {NULL}};

  int epfd = epoll_create(1);
  struct epoll_event event;

  lua_State *L = lbind_new_state(mem_pool);
  int conn_timeout = get_cnf_int_val(cnf, 1, "conn_timeout");
  const char *lfile = get_cnf_str_val(cnf, 1, "worker");

  as_thread_res_t *cfd_ptr = mpf_alloc(mem_pool, sizeof_thread_res(int));
  cfd_ptr->type = AS_TRES_FD;
  cfd_ptr->th = NULL;
  *((int *)cfd_ptr->d) = cfd;

  event.data.ptr = cfd_ptr;
  event.events = EPOLLIN | EPOLLET;
  epoll_ctl(epfd, EPOLL_CTL_ADD, cfd, &event);

  lbind_init_state(L, mem_pool);
  lbind_append_lua_cpath(L, get_cnf_str_val(cnf, 1, "lua_cpath"));
  lbind_append_lua_path(L, get_cnf_str_val(cnf, 1, "lua_path"));
  lbind_reg_value_int(L, LRK_SERVER_EPFD, epfd);
  lbind_ref_lcode_chunk(L, lfile);

  as_thread_array_t *stop_threads = mpf_alloc(
      mem_pool, sizeof_thread_array(_MAX_EVENTS));
  while (keep_running) {
    stop_threads->n = 0;
  }

  mpf_recycle(stop_threads);
  mpf_recycle(cfd_ptr);
  lua_close(L);
  mpf_destroy(mem_pool);
}


void
worker_process2(int channel_fd, as_lua_pconf_t *cnf) {
  process(channel_fd, cnf, 0);
}


void
test_worker_process2(int fd, as_lua_pconf_t *cnf) {
  process(fd, cnf, 1);
}
