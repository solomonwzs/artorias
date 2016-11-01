#ifndef __EPOLL_SERVER_H__
#define __EPOLL_SERVER_H__

#include "server.h"
#include "mem_pool.h"
#include "wrap_conn.h"
#include <sys/epoll.h>

extern void
epoll_server(int fd);

extern void
epoll_server2(int fd);

#endif
