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


static inline void
add_wrap_conn_event(as_rb_conn_t *wc, int epfd) {
  struct epoll_event e;
  set_non_block(wc->fd);
  e.data.ptr = wc;
  e.events = EPOLLIN | EPOLLET;
  epoll_ctl(epfd, EPOLL_CTL_ADD, wc->fd, &e);
}


static inline void
init_mem_pool(as_mem_pool_fixed_t **mp) {
  size_t fs[] = {8, 12, 16, 24, 32, 48, 64, 128, 256, 384, 512, 768, 1064};
  *mp = mpf_new(fs, sizeof(fs) / sizeof(fs[0]));
}


static inline void
init_epfd(int *epfd, as_rb_conn_t *cfd_conn, int cfd) {
  *epfd = epoll_create(1);
  cfd_conn->fd = cfd;
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
conn_close(as_rb_conn_t *wc) {
  if (wc->T != NULL) {
    lbind_unref_fd_lthread(wc->T, wc->fd);
  }
  close(wc->fd);
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
  as_rb_conn_t *wc = container_of(n, as_rb_conn_t, ut_idx);
  debug_log("close: %d\n", wc->fd);\
  handler_close(wc);
  conn_close(wc);
  mpf_recycle(wc);
}


static inline void
close_wrap_conn(as_rb_conn_pool_t *cp, as_rb_conn_t *wc) {
  debug_log("close: %d\n", wc->fd);
  conn_close(wc);
  rb_conn_pool_delete(cp, wc);
  mpf_recycle(wc);
}


static volatile int keep_running = 1;
static void
init_handler(int dummy) {
  keep_running = 0;
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
      if (n_res == 2 && lua_isinteger(T, -2) &&
          lua_tointeger(T, -2) == LAS_WAIT_FOR_INPUT) {
        lua_pop(T, 2);

        rb_conn_pool_insert(cp, new_wc);
        add_wrap_conn_event(new_wc, epfd);
        continue;
      }
    } else if (ret != LUA_OK) {
      lb_pop_error_msg(T);
    }

    debug_log("close: %d\n", new_wc->fd);
    conn_close(new_wc);
    mpf_recycle(new_wc);
  }
}


static inline void
handler_read(as_rb_conn_t *wc, as_rb_conn_pool_t *conn_pool, int epfd,
             struct epoll_event *event) {
  lua_State *T = wc->T;
  int n = lua_gettop(T);

  lua_pushinteger(T, LAS_READY_TO_INPUT);
  lua_pushinteger(T, wc->fd);
  int ret = alua_resume(T, 2);

  if (ret == LUA_YIELD) {
    int n_res = lua_gettop(T) - n;
    if (n_res == 2 && lua_isinteger(T, -2)){
      int status = lua_tointeger(T, -2);
      lua_pop(T, 2);

      if (status == LAS_WAIT_FOR_INPUT) {
        rb_conn_update_ut(conn_pool, wc);
        return;
      } else if (status == LAS_WAIT_FOR_OUTPUT) {
        rb_conn_update_ut(conn_pool, wc);

        struct epoll_event e;
        e.data.ptr = wc;
        e.events = EPOLLOUT | EPOLLET;
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
  lua_pushinteger(T, wc->fd);
  int ret = alua_resume(T, 2);

  if (ret == LUA_YIELD) {
    int n_res = lua_gettop(T) - n;
    if (n_res == 2 && lua_isinteger(T, -2)) {
      int status = lua_tointeger(T, -2);
      lua_pop(T, 2);

      if (status == LAS_WAIT_FOR_INPUT) {
        rb_conn_update_ut(conn_pool, wc);

        struct epoll_event e;
        e.data.ptr = wc;
        e.events = EPOLLIN | EPOLLET;
        epoll_ctl(epfd, EPOLL_CTL_MOD, wc->fd, &e);

        return;
      } else if (status == LAS_WAIT_FOR_OUTPUT) {
        rb_conn_update_ut(conn_pool, wc);
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
  as_rb_conn_t cfd_conn;
  struct epoll_event events[100];
  int conn_timeout = get_cnf_int_val(cnf, 1, "conn_timeout");
  lua_State *L;
  const char *lfile = get_cnf_str_val(cnf, 1, WORKER_FILE_NAME);

  init_mem_pool(&mem_pool);
  init_epfd(&epfd, &cfd_conn, fd);
  init_lua_state(&L, mem_pool, cnf, epfd);

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
  rb_tree_postorder_travel(&conn_pool.ut_tree, recycle_conn);
  lua_close(L);
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

