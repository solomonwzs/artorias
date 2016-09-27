#ifndef __EPOLL_SERVER_H__
#define __EPOLL_SERVER_H__

#include "server.h"
#include <stdio.h>
#include <unistd.h>
#include <time.h>
#include <sys/epoll.h>

extern void
epoll_server(int fd);

#endif
