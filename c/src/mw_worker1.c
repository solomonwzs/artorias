#include "mw_worker.h"
#include "mem_pool.h"
#include <sys/epoll.h>
#include <signal.h>
#include "server.h"
#include "lua_bind.h"
#include "lua_utils.h"

#define WORKER_FILE_NAME "worker"

#define get_cnf_int_val(_cnf_, _n_, ...) ({\
  as_cnf_return_t __ret = lpconf_get_pconf_value(_cnf_, _n_, ## __VA_ARGS__);\
  __ret.val.i;\
})

#define get_cnf_str_val(_cnf_, _n_, ...) ({\
  as_cnf_return_t __ret = lpconf_get_pconf_value(_cnf_, _n_, ## __VA_ARGS__);\
  __ret.val.s;\
})

#define init_mem_pool(_mp_) do {\
  size_t __fs[] = {8, 12, 16, 24, 32, 48, 64, 128, 256, 384, 512, 768, 1064};\
  (_mp_) = mpf_new(__fs, sizeof(__fs) / sizeof(__fs[0]));\
} while (0)

#define init_epfd(_epfd_, _cfd_) do {\
  as_rb_conn_t __cfd_conn;\
  _epfd_ = epoll_create(1);\
  __cfd_conn.fd = _cfd_;\
  add_wrap_conn_event(&__cfd_conn, _epfd_);\
} while (0)

#define init_lua_state(_L_, _mp_, _cnf_, _epfd_) do {\
  _L_ = lbind_new_state(_mp_);\
  lbind_init_state(_L_, _mp_);\
  lbind_append_lua_cpath(_L_, get_cnf_str_val(_cnf_, 1, "lua_cpath"));\
  lbind_append_lua_path(_L_, get_cnf_str_val(_cnf_, 1, "lua_path"));\
  lbind_reg_integer_value(_L_, LRK_SERVER_EPFD, _epfd_);\
  lbind_ref_lcode_chunk(_L_, get_cnf_str_val(_cnf_, 1, WORKER_FILE_NAME));\
} while (0)

#define recycle_conn(_n_) do {\
  as_rb_conn_t *__wc = container_of(_n_, as_rb_conn_t, ut_idx);\
  debug_log("close: %d\n", __wc->fd);\
  handler_close(__wc);\
  rb_conn_close(__wc);\
  mpf_recycle(__wc);\
} while (0)

#define close_wrap_conn(_cp_, _wc_) do {\
  debug_log("close: %d\n", (_wc_)->fd);\
  rb_conn_close(_wc_);\
  rb_conn_pool_delete(_cp_, _wc_);\
  mpf_recycle(_wc_);\
} while (0)

#define add_wrap_conn_event(_wc_, _epfd_) do {\
  struct epoll_event __e;\
  set_non_block((_wc_)->fd);\
  __e.data.ptr = (_wc_);\
  __e.events = EPOLLIN | EPOLLET;\
  epoll_ctl((_epfd_), EPOLL_CTL_ADD, (_wc_)->fd, &__e);\
} while (0)


static volatile int keep_running = 1;
static void
init_handler(int dummy) {
  keep_running = 0;
}


static inline void
handler_close(as_rb_conn_t *wc) {
  lua_State *T = wc->T;
  lua_pushinteger(T, LAS_SOCKET_CLOSEED);
  alua_resume(T, 1);
}


static inline void
handler_error(as_rb_conn_t *wc, int fd, as_rb_conn_pool_t *cp) {
  if (errno != EAGAIN && errno != EWOULDBLOCK) {
    debug_perror("epoll_wait");
  }
  if (wc->fd == fd) {
    close(wc->fd);
  } else {
    close_wrap_conn(cp, wc);
  }
}


static inline void
handler_accept(int fd, as_rb_conn_pool_t *cp, as_mem_pool_fixed_t *mp,
               int epfd, lua_State *L, const char *lfile, int single_mode) {
  int (*new_fd_func)(int);
  if (single_mode) {
    new_fd_func = new_accept_fd;
  } else {
    new_fd_func = recv_fd_from_socket;
  }

  while (1) {
    int infd = new_fd_func(fd);
    if (infd == -1) {
      break;
    }

    as_rb_conn_t *new_wc = mpf_alloc(mp, sizeof(as_rb_conn_t));
    rb_conn_init(new_wc, infd);
    lua_State *T = lbind_new_fd_lthread(L, infd);
    new_wc->T = T;
    int n = lua_gettop(T);

    lbind_get_lcode_chunk(new_wc->T, lfile);
    int ret = alua_resume(T, 0);

    if (ret == LUA_YIELD) {
      int n_res = lua_gettop(T) - n;
      if (n_res == 1 && lua_isinteger(T, -1) &&
          lua_tointeger(T, -1) == LAS_WAIT_FOR_INPUT) {
        lua_pop(T, 1);
        rb_conn_pool_insert(cp, new_wc);
        add_wrap_conn_event(new_wc, epfd);
        continue;
      }
    } else if (ret != LUA_OK) {
      lb_pop_error_msg(T);
    }

    debug_log("close: %d\n", new_wc->fd);
    rb_conn_close(new_wc);
    mpf_recycle(new_wc);
  }
}


static inline void
handler_read(as_rb_conn_t *wc, as_rb_conn_pool_t *conn_pool, int epfd,
             struct epoll_event *event) {
  lua_State *T = wc->T;
  int n = lua_gettop(T);

  lua_pushinteger(T, LAS_READY_TO_INPUT);
  int ret = alua_resume(T, 1);

  if (ret == LUA_YIELD) {
    int n_res = lua_gettop(T) - n;
    if (n_res == 1 && lua_isinteger(T, -1)){
      int status = lua_tointeger(T, -1);
      lua_pop(T, 1);
      if (status == LAS_WAIT_FOR_INPUT) {
        rb_conn_pool_update_conn_ut(conn_pool, wc);
        return;
      } else if (status == LAS_WAIT_FOR_OUTPUT) {
        rb_conn_pool_update_conn_ut(conn_pool, wc);

        struct epoll_event e;
        e.data.ptr = wc;
        e.events = event->events | EPOLLOUT;
        epoll_ctl(epfd, EPOLL_CTL_MOD, wc->fd, &e);

        return;
      }
    }
  } else if (ret != LUA_OK) {
    lb_pop_error_msg(T);
  }
  close_wrap_conn(conn_pool, wc);
}


static inline void
handler_write(as_rb_conn_t *wc, as_rb_conn_pool_t *conn_pool, int epfd,
              struct epoll_event *event) {
  lua_State *T = wc->T;
  int n = lua_gettop(T);

  lua_pushinteger(T, LAS_READY_TO_OUTPUT);
  int ret = alua_resume(T, 1);

  if (ret == LUA_YIELD) {
    int n_res = lua_gettop(T) - n;
    if (n_res == 1 && lua_isinteger(T, -1)) {
      int status = lua_tointeger(T, -1);
      lua_pop(T, 1);
      if (status == LAS_WAIT_FOR_INPUT) {
        rb_conn_pool_update_conn_ut(conn_pool, wc);

        struct epoll_event e;
        e.data.ptr = wc;
        e.events = EPOLLIN | EPOLLET;
        epoll_ctl(epfd, EPOLL_CTL_MOD, wc->fd, &e);

        return;
      } else if (status == LAS_WAIT_FOR_OUTPUT) {
        rb_conn_pool_update_conn_ut(conn_pool, wc);
        return;
      }
    }
  } else if (ret != LUA_OK) {
    lb_pop_error_msg(T);
  }
  close_wrap_conn(conn_pool, wc);
}


static inline void
remove_time_out_conn(as_rb_conn_pool_t *cp, int timeout) {
  as_rb_tree_t ot;
  ot.root = rb_conn_remove_timeout_conn(cp, timeout);
  rb_tree_postorder_travel(&ot, recycle_conn);
}


static void
process(int fd, as_lua_pconf_t *cnf, int single_mode) {
  if (single_mode) {
    signal(SIGINT, init_handler);
  }

  as_mem_pool_fixed_t *mem_pool;
  int epfd;
  as_rb_conn_pool_t conn_pool = NULL_RB_CONN_POOL;
  struct epoll_event events[100];
  int conn_timeout = get_cnf_int_val(cnf, 1, "conn_timeout");
  lua_State *L;
  const char *lfile = get_cnf_str_val(cnf, 1, WORKER_FILE_NAME);

  init_mem_pool(mem_pool);
  init_epfd(epfd, fd);
  init_lua_state(L, mem_pool, cnf, epfd);

  while (keep_running) {
    int active_cnt = epoll_wait(epfd, events, 100, 1*1000);
    for (int i = 0; i < active_cnt; ++i) {
      as_rb_conn_t *wc = (as_rb_conn_t *)events[i].data.ptr;
      if (events[i].events & EPOLLERR || events[i].events & EPOLLHUP ||
          !(events[i].events & EPOLLIN || events[i].events & EPOLLOUT)) {
        handler_error(wc, fd, &conn_pool);
      } else if (wc->fd == fd) {
        handler_accept(fd, &conn_pool, mem_pool, epfd, L, lfile, single_mode);
      } else if (events[i].events & EPOLLIN) {
        handler_read(wc, &conn_pool, epfd, &events[i]);
      } else if (events[i].events & EPOLLOUT) {
        handler_write(wc, &conn_pool, epfd, &events[i]);
      }
    }
    remove_time_out_conn(&conn_pool, conn_timeout);
    // if (active_cnt == 0) {
    //   lua_gc(L, LUA_GCCOLLECT, 0);
    //   lbind_check_metatable_elem_by_tname(L, LRK_THREAD_LOCAL_VAR_TABLE);
    // }
  }
  lua_close(L);
  rb_tree_postorder_travel(&conn_pool.ut_tree, recycle_conn);
  mpf_destroy(mem_pool);
}


void
worker_process1(int channel_fd, as_lua_pconf_t *cnf) {
  process(channel_fd, cnf, 0);
}

void
test_worker_process1(int fd, as_lua_pconf_t *cnf) {
  process(fd, cnf, 1);
}
