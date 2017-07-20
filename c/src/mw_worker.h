#ifndef __MW_WORKER_H__
#define __MW_WORKER_H__

#include "lua_pconf.h"
#include "thread.h"
#include "mem_pool.h"

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
} as_mw_worker_ctx_t;

extern void
worker_process(int channel_fd, as_lua_pconf_t *cnf);

extern void
test_worker_process(int fd, as_lua_pconf_t *cnf);

#endif
