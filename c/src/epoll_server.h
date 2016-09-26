#ifndef __EPOLL_SERVER_H__
#define __EPOLL_SERVER_H__

#include "server.h"
#include <stdio.h>
#include <unistd.h>
#include <time.h>
#include <sys/epoll.h>

struct myevent_s {
  int fd;
  int events;
  void *arg;
  void (*call_back)(int fd, int events, void *arg);

  int status;
  int len;
  long last_active;
};

extern void
epoll_server(int fd);

#endif
