#ifndef __MW_WORKER_H__
#define __MW_WORKER_H__

#include "lua_pconf.h"
#include "wrap_conn.h"

extern void
worker_process0(int channel_fd, as_lua_pconf_t *cnf);

extern void
worker_process1(int channel_fd, as_lua_pconf_t *cnf);

extern void
test_worker_process1(int fd, as_lua_pconf_t *cnf);

extern void
worker_process2(int channel_fd, as_lua_pconf_t *cnf);

extern void
test_worker_process2(int fd, as_lua_pconf_t *cnf);

#endif
