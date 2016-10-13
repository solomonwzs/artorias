#ifndef __SERVER_H__
#define __SERVER_H__

#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <arpa/inet.h>
#include <errno.h>
#include <fcntl.h>
#include <string.h>

#define DEBUG
#ifdef DEBUG
#define debug_log(_fmt_, ...) \
    fprintf(stderr, "\033[0;33m[%s:%d:%s] %d\033[0m " _fmt_, __FILE__, \
            __LINE__, __func__, getpid(), ## __VA_ARGS__)
#define debug_perror(_s_) debug_log("%s: %s\n", _s_, strerror(errno))
#else
#define debug_log(_fmt_, ...)
#define debug_perror(_s_) perror(_s_)
#endif


extern int
new_accept_fd(int fd);

extern int
read_from_client(int fd);

extern int
make_socket(uint16_t port);

extern int
set_non_block(int fd);

extern int
send_fd_by_socket(int socket, int fd);

extern int
recv_fd_from_socket(int socket);

#endif
