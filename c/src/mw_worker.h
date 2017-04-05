#ifndef __MW_WORKER_H__
#define __MW_WORKER_H__

#include "lua_pconf.h"
#include "wrap_conn.h"

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


extern void
worker_process0(int channel_fd, as_lua_pconf_t *cnf);

extern void
worker_process1(int channel_fd, as_lua_pconf_t *cnf);

extern void
test_worker_process1(int fd, as_lua_pconf_t *cnf);

#endif
