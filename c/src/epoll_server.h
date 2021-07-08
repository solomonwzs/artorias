#ifndef __EPOLL_SERVER_H__
#define __EPOLL_SERVER_H__

#include <unistd.h>

#include "mem_pool.h"
#include "utils.h"

extern void epoll_server(int fd);

extern void epoll_server2(int fd);

#endif
