#ifndef __EPOLL_SERVER_H__
#define __EPOLL_SERVER_H__

#include <sys/epoll.h>
#include <stdio.h>
#include <unistd.h>

#include "server.h"

extern void
epoll_server(int fd);

#endif
