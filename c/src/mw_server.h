#ifndef __MW_SERVER__
#define __MW_SERVER__

#include "server.h"
#include "mem_pool.h"
#include "wrap_conn.h"
#include <sys/epoll.h>

extern void
master_workers_server(int fd, int n);

#endif
