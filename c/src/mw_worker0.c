#include "mw_worker.h"
#include "mem_pool.h"
#include <sys/epoll.h>
#include "server.h"

#define get_cnf_int_val(_cnf_, _n_, _field_) ({\
  as_cnf_return_t __ret = lpconf_get_pconf_value(_cnf_, _n_, _field_);\
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

#define handle_error(_wc_, _cfd_, _cp_) do {\
  if (errno != EAGAIN && errno != EWOULDBLOCK) {\
    debug_perror("epoll_wait");\
  }\
  if ((_wc_)->fd == (_cfd_)) {\
    close((_wc_)->fd);\
  } else {\
    close_wrap_conn(&(_cp_), _wc_);\
  }\
} while(0)

#define handle_accept(_cfd_, _cp_, _mp_, _epfd_) while (1) {\
  int __infd = recv_fd_from_socket(_cfd_);\
  if (__infd == -1) {\
    break;\
  }\
  as_rb_conn_t *__nwc = mpf_alloc(_mp_, sizeof(as_rb_conn_t));\
  rb_conn_init(NULL, __nwc, __infd);\
  rb_conn_pool_insert(&(_cp_), __nwc);\
  add_wrap_conn_event(__nwc, _epfd_);\
}

#define remove_time_out_conn(_cp_, _tsecs_) do {\
  as_rb_tree_t __ot;\
  __ot.root = rb_conn_remove_timeout_conn(&(_cp_), _tsecs_);\
  rb_tree_postorder_travel(&__ot, recycle_conn);\
} while(0)


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
        handle_error(wc, channel_fd, conn_pool);
      } else if (wc->fd == channel_fd) {
        handle_accept(channel_fd, conn_pool, mem_pool, epfd);
      } else if (events[i].events & EPOLLIN) {
        int n = simple_read_from_client(wc->fd);
        if (n <= 0) {
          close_wrap_conn(&conn_pool, wc);
        } else {
          rb_conn_pool_update_conn_ut(&conn_pool, wc);
          write(wc->fd, "+OK\r\n", 5);
        }
      }
    }
    remove_time_out_conn(conn_pool, conn_timeout);
  }
  rb_tree_postorder_travel(&conn_pool.ut_tree, recycle_conn);
  mpf_destroy(mem_pool);
}
