#ifndef __SERVER_H__
#define __SERVER_H__

#include <stdio.h>
#include <sys/socket.h>
#include <stdlib.h>
#include <netinet/in.h>
#include <unistd.h>
#include <arpa/inet.h>
#include <errno.h>
#include <sys/epoll.h>

#define debug_log(_fmt_, ...) \
    printf("\033[0;33m[%s:%d]\033[0m " _fmt_, __FILE__, __LINE__, ## __VA_ARGS__)

#endif
