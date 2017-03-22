#ifndef __EPOLL_SERVER_H__
#define __EPOLL_SERVER_H__

#include "mem_pool.h"
#include "utils.h"
#include <unistd.h>

extern void
epoll_server(int fd);

extern void
epoll_server2(int fd);

#endif
