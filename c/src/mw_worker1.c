#include "mw_worker.h"
#include "mem_pool.h"
#include <sys/epoll.h>
#include "server.h"
#include "lua_bind.h"
#include "lua_utils.h"

#define WORKER_FILE_NAME "worker"

#define H_ACCEPT  0
#define H_READ    1
#define H_WRITE   2

#define get_cnf_int_val(_cnf_, _n_, ...) ({\
  as_cnf_return_t __ret = lpconf_get_pconf_value(_cnf_, _n_, ## __VA_ARGS__);\
  __ret.val.i;\
})

#define get_cnf_str_val(_cnf_, _n_, ...) ({\
  as_cnf_return_t __ret = lpconf_get_pconf_value(_cnf_, _n_, ## __VA_ARGS__);\
  __ret.val.s;\
})

#define init_mem_pool(_mp_) do {\
  size_t __fs[] = {8, 12, 16, 24, 32, 48, 64, 128, 256, 384, 512, 768, 1024};\
  (_mp_) = mpf_new(__fs, sizeof(__fs) / sizeof(__fs[0]));\
} while (0)

#define init_epfd(_epfd_, _cfd_) do {\
  as_rb_conn_t __cfd_conn;\
  _epfd_ = epoll_create(1);\
  __cfd_conn.fd = _cfd_;\
  add_wrap_conn_event(&__cfd_conn, _epfd_);\
} while (0)

#define init_lua_state(_L_, _mp_, _cnf_) do {\
  _L_ = lbind_new_state(_mp_);\
  lbind_init_state(_L_, _mp_);\
  lbind_append_lua_cpath(_L_, get_cnf_str_val(_cnf_, 1, "cpath"));\
  lbind_ref_lcode_chunk(_L_, get_cnf_str_val(_cnf_, 1, WORKER_FILE_NAME));\
} while (0)


static inline void
handler_error(as_rb_conn_t *wc, int channel_fd, as_rb_conn_pool_t *cp) {
  if (errno != EAGAIN && errno != EWOULDBLOCK) {
    debug_perror("epoll_wait");
  }
  if (wc->fd == channel_fd) {
    close(wc->fd);
  } else {
    close_wrap_conn(cp, wc);
  }
}


static inline void
handler_accept(int channel_fd, as_rb_conn_pool_t *cp, as_mem_pool_fixed_t *mp,
               int epfd, lua_State *L, const char *lfile) {
  while (1) {
    int infd = recv_fd_from_socket(channel_fd);
    if (infd == -1) {
      break;
    }

    as_rb_conn_t *new_wc = mpf_alloc(mp, sizeof(as_rb_conn_t));
    rb_conn_init(new_wc, infd, L);

    lbind_get_lcode_chunk(new_wc->T, lfile);
    int ret = lua_resume(new_wc->T, NULL, 0);
    if (ret != LUA_YIELD) {
      debug_log("close: %d\n", new_wc->fd);
      rb_conn_close(new_wc);
      mpf_recycle(new_wc);
    } else {
      rb_conn_pool_insert(cp, new_wc);
      add_wrap_conn_event(new_wc, epfd);
    }
  }
}


static inline void
handler_read(as_rb_conn_t *wc, as_rb_conn_pool_t *conn_pool) {
  int ret = lua_resume(wc->T, NULL, 0);
  if (ret == LUA_OK) {
    close_wrap_conn(conn_pool, wc);
  } else if (ret == LUA_YIELD) {
    rb_conn_pool_update_conn_ut(conn_pool, wc);
  } else {
    lb_pop_error_msg(wc->T);
    close_wrap_conn(conn_pool, wc);
  }
}


static inline void
remove_time_out_conn(as_rb_conn_pool_t *cp, int timeout) {
  as_rb_tree_t ot;
  ot.root = rb_conn_remove_timeout_conn(cp, timeout);
  rb_tree_postorder_travel(&ot, recycle_conn);
}


void
worker_process1(int channel_fd, as_lua_pconf_t *cnf) {
  as_mem_pool_fixed_t *mem_pool;
  int epfd;
  as_rb_conn_pool_t conn_pool = NULL_RB_CONN_POOL;
  struct epoll_event events[100];
  int conn_timeout = get_cnf_int_val(cnf, 1, "conn_timeout");
  lua_State *L;
  const char *lfile = get_cnf_str_val(cnf, 1, WORKER_FILE_NAME);

  init_mem_pool(mem_pool);
  init_epfd(epfd, channel_fd);
  init_lua_state(L, mem_pool, cnf);

  while (1) {
    int active_cnt = epoll_wait(epfd, events, 100, 1*1000);
    for (int i = 0; i < active_cnt; ++i) {
      as_rb_conn_t *wc = (as_rb_conn_t *)events[i].data.ptr;
      if (events[i].events & EPOLLERR || events[i].events & EPOLLHUP ||
          !(events[i].events & EPOLLIN)) {
        handler_error(wc, channel_fd, &conn_pool);
      } else if (wc->fd == channel_fd) {
        handler_accept(channel_fd, &conn_pool, mem_pool, epfd, L, lfile);
      } else if (events[i].events & EPOLLIN) {
        handler_read(wc, &conn_pool);
      }
    }
    remove_time_out_conn(&conn_pool, conn_timeout);
  }
  rb_tree_postorder_travel(&conn_pool.ut_tree, recycle_conn);
  mpf_destroy(mem_pool);
}
