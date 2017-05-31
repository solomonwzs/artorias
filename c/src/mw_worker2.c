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

#define wrap_conn_fd(_wc_) \
    ((_wc_)->type == AS_WC_RB_CONN ? \
     ((as_rb_conn_t *)(_wc_)->d)->fd : \
     ((as_sl_conn_t *)(_wc_)->d)->fd)


static inline void
add_wrap_conn_event(as_wrap_conn_t *wc, int epfd) {
  struct epoll_event e;
  int fd;
  if (wc->type == AS_WC_RB_CONN) {
    as_rb_conn_t *rc = (as_rb_conn_t *)wc->d;
    fd = rc->fd;
  } else {
    as_sl_conn_t *sc = (as_sl_conn_t *)wc->d;
    fd = sc->fd;
  }
  set_non_block(fd);
  e.data.ptr = wc;
  e.events = EPOLLIN | EPOLLET;
  epoll_ctl(epfd, EPOLL_CTL_ADD, fd, &e);
}


static inline void
init_mem_pool(as_mem_pool_fixed_t **mp) {
  size_t fs[] = {8, 12, 16, 24, 32, 48, 64, 128, 256, 384, 512, 768, 1064};
  *mp = mpf_new(fs, sizeof(fs) / sizeof(fs[0]));
}


static inline void
init_epfd(int *epfd, as_wrap_conn_t *cfd_conn, int cfd) {
  *epfd = epoll_create(1);
  as_sl_conn_t *sc = (as_sl_conn_t *)cfd_conn->d;
  sc->fd = cfd;
  add_wrap_conn_event(cfd_conn, *epfd);
}


static inline void
init_lua_state(lua_State **L, as_mem_pool_fixed_t *mp, as_lua_pconf_t *cnf,
                int epfd) {
  *L = lbind_new_state(mp);
  lbind_init_state(*L, mp);
  lbind_append_lua_cpath(*L, get_cnf_str_val(cnf, 1, "lua_cpath"));
  lbind_append_lua_path(*L, get_cnf_str_val(cnf, 1, "lua_path"));
  lbind_reg_integer_value(*L, LRK_SERVER_EPFD, epfd);
  lbind_ref_lcode_chunk(*L, get_cnf_str_val(cnf, 1, WORKER_FILE_NAME));
}


static inline void
rb_conn_close(as_rb_conn_t *rc) {
  if (rc->T != NULL) {
    lbind_unref_fd_lthread(rc->T, rc->fd);
  }
  close(rc->fd);
}


static inline void
handler_close(as_rb_conn_t *wc) {
  lua_State *T = wc->T;
  lua_pushinteger(T, LAS_SOCKET_CLOSEED);
  lua_pushinteger(T, wc->fd);
  alua_resume(T, 2);
}


static inline void
recycle_conn(as_rb_node_t *n) {
  as_rb_conn_t *rc = container_of(n, as_rb_conn_t, ut_idx);
  debug_log("close: %d\n", rc->fd);\
  handler_close(rc);
  rb_conn_close(rc);

  as_wrap_conn_t *wc = container_of((void *)rc, as_wrap_conn_t, d);
  mpf_recycle(wc);
}


static inline void
close_wrap_rb_conn(as_rb_conn_pool_t *cp, as_wrap_conn_t *wc) {
  as_rb_conn_t *rc = (as_rb_conn_t *)wc->d;
  debug_log("close: %d\n", rc->fd);
  rb_conn_close(rc);
  rb_conn_pool_delete(cp, rc);
  mpf_recycle(wc);
}


static volatile int keep_running = 1;
static void
init_handler(int dummy) {
  keep_running = 0;
}


static inline void
handler_error(as_wrap_conn_t *wc, int cfd, as_rb_conn_pool_t *cp) {
  if (errno != EAGAIN && errno != EWOULDBLOCK) {
    debug_perror("epoll_wait");
  }
  if (wc->type == AS_WC_RB_CONN) {
    close_wrap_rb_conn(cp, wc);
  } else {
    as_sl_conn_t *sc = (as_sl_conn_t *)wc->d;
    if (sc->fd == cfd) {
      close(sc->fd);
    }
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

    as_wrap_conn_t *new_wc = (as_wrap_conn_t *)mpf_alloc(
        mp, sizeof_wrap_conn(as_rb_conn_t));
    new_wc->type = AS_WC_RB_CONN;
    as_rb_conn_t *rc = (as_rb_conn_t *)new_wc->d;

    rb_conn_init(rc, infd);
    lua_State *T = lbind_new_fd_lthread(L, infd);
    rc->T = T;
    int n = lua_gettop(T);

    lbind_get_lcode_chunk(rc->T, lfile);
    int ret = alua_resume(T, 0);

    if (ret == LUA_YIELD) {
      int n_res = lua_gettop(T) - n;
      if (n_res == 2 && lua_isinteger(T, -2) &&
          lua_tointeger(T, -2) == LAS_WAIT_FOR_INPUT) {
        lua_pop(T, 2);

        rb_conn_pool_insert(cp, rc);
        add_wrap_conn_event(new_wc, epfd);
        continue;
      }
    } else if (ret != LUA_OK) {
      lb_pop_error_msg(T);
    }

    debug_log("close: %d\n", rc->fd);
    rb_conn_close(rc);
    mpf_recycle(new_wc);
  }
}


static inline void
handler_rb_conn_read(as_wrap_conn_t *wc, as_rb_conn_pool_t *conn_pool,
                     int epfd, struct epoll_event *event) {
  as_rb_conn_t *rc = (as_rb_conn_t *)wc->d;
  lua_State *T = rc->T;
  int n = lua_gettop(T);

  lua_pushinteger(T, LAS_READY_TO_INPUT);
  lua_pushinteger(T, rc->fd);
  int ret = alua_resume(T, 2);

  if (ret == LUA_YIELD) {
    int n_res = lua_gettop(T) - n;
    if (n_res == 2 && lua_isinteger(T, -2)){
      int status = lua_tointeger(T, -2);
      lua_pop(T, 2);

      if (status == LAS_WAIT_FOR_INPUT) {
        rb_conn_update_ut(conn_pool, rc);
        return;
      } else if (status == LAS_WAIT_FOR_OUTPUT) {
        rb_conn_update_ut(conn_pool, rc);

        struct epoll_event e;
        e.data.ptr = wc;
        e.events = EPOLLOUT | EPOLLET;
        epoll_ctl(epfd, EPOLL_CTL_MOD, rc->fd, &e);

        return;
      }
    }
  } else if (ret != LUA_OK) {
    lb_pop_error_msg(T);
  }
  close_wrap_rb_conn(conn_pool, wc);
}


static inline void
handler_rb_conn_write(as_wrap_conn_t *wc, as_rb_conn_pool_t *conn_pool,
                      int epfd, struct epoll_event *event) {
  as_rb_conn_t *rc = (as_rb_conn_t *)wc->d;
  lua_State *T = rc->T;
  int n = lua_gettop(T);

  lua_pushinteger(T, LAS_READY_TO_OUTPUT);
  lua_pushinteger(T, rc->fd);
  int ret = alua_resume(T, 2);

  if (ret == LUA_YIELD) {
    int n_res = lua_gettop(T) - n;
    if (n_res == 2 && lua_isinteger(T, -2)) {
      int status = lua_tointeger(T, -2);
      lua_pop(T, 2);

      if (status == LAS_WAIT_FOR_INPUT) {
        rb_conn_update_ut(conn_pool, rc);

        struct epoll_event e;
        e.data.ptr = wc;
        e.events = EPOLLIN | EPOLLET;
        epoll_ctl(epfd, EPOLL_CTL_MOD, rc->fd, &e);

        return;
      } else if (status == LAS_WAIT_FOR_OUTPUT) {
        rb_conn_update_ut(conn_pool, rc);
        return;
      }
    }
  } else if (ret != LUA_OK) {
    lb_pop_error_msg(T);
  }
  close_wrap_rb_conn(conn_pool, wc);
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

  init_mem_pool(&mem_pool);

  as_wrap_conn_t *cfd_conn = (as_wrap_conn_t *)mpf_alloc(
      mem_pool, sizeof_wrap_conn(as_sl_conn_t));
  cfd_conn->type = AS_WC_SL_CONN;

  init_epfd(&epfd, cfd_conn, fd);
  init_lua_state(&L, mem_pool, cnf, epfd);

  while (keep_running) {
    int active_cnt = epoll_wait(epfd, events, 100, 1*1000);
    for (int i = 0; i < active_cnt; ++i) {
      as_wrap_conn_t *wc = (as_wrap_conn_t *)events[i].data.ptr;
      if (events[i].events & EPOLLERR || events[i].events & EPOLLHUP ||
          !(events[i].events & EPOLLIN || events[i].events & EPOLLOUT)) {
        handler_error(wc, fd, &conn_pool);
      } else if (wc == cfd_conn) {
        handler_accept(fd, &conn_pool, mem_pool, epfd, L, lfile, single_mode);
      } else if (events[i].events & EPOLLIN) {
        if (wc->type == AS_WC_RB_CONN) {
          handler_rb_conn_read(wc, &conn_pool, epfd, &events[i]);
        }
      } else if (events[i].events & EPOLLOUT) {
        if (wc->type == AS_WC_RB_CONN) {
          handler_rb_conn_write(wc, &conn_pool, epfd, &events[i]);
        }
      }
    }
    remove_time_out_conn(&conn_pool, conn_timeout);
    // if (active_cnt == 0) {
    //   lua_gc(L, LUA_GCCOLLECT, 0);
    //   lbind_check_metatable_elem_by_tname(L, LRK_THREAD_LOCAL_VAR_TABLE);
    // }
  }
  rb_tree_postorder_travel(&conn_pool.ut_tree, recycle_conn);
  lua_close(L);
  mpf_recycle(cfd_conn);
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
