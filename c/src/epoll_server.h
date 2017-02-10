#ifndef __EPOLL_SERVER_H__
#define __EPOLL_SERVER_H__

#include "mem_pool.h"
#include "utils.h"
#include <unistd.h>

#define close_wrap_conn(_L_, _cp_, _wc_) do {\
  debug_log("close: %d\n", (_wc_)->fd);\
  rb_conn_close(_L_, _wc_);\
  rb_conn_pool_delete(_cp_, _wc_);\
  mpf_recycle(_wc_);\
} while (0)

#define add_wrap_conn_event(_wc_, _e_, _epfd_) do {\
  set_non_block((_wc_)->fd);\
  (_e_).data.ptr = (_wc_);\
  (_e_).events = EPOLLIN | EPOLLET;\
  epoll_ctl((_epfd_), EPOLL_CTL_ADD, (_wc_)->fd, &(_e_));\
} while (0)

#define recycle_conn(_n_) do {\
  as_rb_conn_t *__wc = container_of(_n_, as_rb_conn_t, ut_idx);\
  debug_log("close: %d\n", __wc->fd);\
  close(__wc->fd);\
  mpf_recycle(__wc);\
} while (0)

extern void
epoll_server(int fd);

extern void
epoll_server2(int fd);

#endif
