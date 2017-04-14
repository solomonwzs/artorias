#include "mw_worker.h"
#include "mem_pool.h"
#include <sys/epoll.h>
#include "server.h"

#define get_cnf_int_val(_cnf_, _n_, ...) ({\
  as_cnf_return_t __ret = lpconf_get_pconf_value(_cnf_, _n_, ## __VA_ARGS__);\
  __ret.val.i;\
})

#define init_mem_pool(_mp_) do {\
  size_t __fs[] = {8, 12, 16, 24, 32, 48, 64, 128, 256, 384, 512, 768, 1024};\
  (_mp_) = mpf_new(__fs, sizeof(__fs) / sizeof(__fs[0]));\
} while(0)

#define init_epfd(_epfd_, _cfd_) do {\
  as_rb_conn_t __cfd_conn;\
  _epfd_ = epoll_create(1);\
  __cfd_conn.fd = _cfd_;\
  add_wrap_conn_event(&__cfd_conn, _epfd_);\
} while(0)

#define recycle_conn(_n_) do {\
  as_rb_conn_t *__wc = container_of(_n_, as_rb_conn_t, ut_idx);\
  debug_log("close: %d\n", __wc->fd);\
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
               int epfd) {
  while (1) {
    int infd = recv_fd_from_socket(channel_fd);
    if (infd == -1) {
      break;
    }

    as_rb_conn_t *new_wc = mpf_alloc(mp, sizeof(as_rb_conn_t));
    rb_conn_init(new_wc, infd, NULL);
    rb_conn_pool_insert(cp, new_wc);
    add_wrap_conn_event(new_wc, epfd);
  }
}


static inline void
handler_read(as_rb_conn_t *wc, as_rb_conn_pool_t *cp) {
  int n = simple_read_from_client(wc->fd);
  if (n <= 0) {
    close_wrap_conn(cp, wc);
  } else {
    rb_conn_pool_update_conn_ut(cp, wc);
    write(wc->fd, "+OK\r\n", 5);
  }
}


static inline void
remove_time_out_conn(as_rb_conn_pool_t *cp, int timeout) {
  as_rb_tree_t ot;
  ot.root = rb_conn_remove_timeout_conn(cp, timeout);
  rb_tree_postorder_travel(&ot, recycle_conn);
}


void
worker_process0(int channel_fd, as_lua_pconf_t *cnf) {
  as_mem_pool_fixed_t *mem_pool;
  int epfd;
  as_rb_conn_pool_t conn_pool = NULL_RB_CONN_POOL;
  struct epoll_event events[100];
  int conn_timeout = get_cnf_int_val(cnf, 1, "conn_timeout");

  init_mem_pool(mem_pool);
  init_epfd(epfd, channel_fd);

  while (1) {
    int active_cnt = epoll_wait(epfd, events, 100, 1*1000);
    for (int i = 0; i < active_cnt; ++i) {
      as_rb_conn_t *wc = (as_rb_conn_t *)events[i].data.ptr;
      if (events[i].events & EPOLLERR || events[i].events & EPOLLHUP ||
          !(events[i].events & EPOLLIN)) {
        handler_error(wc, channel_fd, &conn_pool);
      } else if (wc->fd == channel_fd) {
        handler_accept(channel_fd, &conn_pool, mem_pool, epfd);
      } else if (events[i].events & EPOLLIN) {
        handler_read(wc, &conn_pool);
      }
    }
    remove_time_out_conn(&conn_pool, conn_timeout);
  }
  rb_tree_postorder_travel(&conn_pool.ut_tree, recycle_conn);
  mpf_destroy(mem_pool);
}
