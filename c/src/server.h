#ifndef __SERVER_H__
#define __SERVER_H__

#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <arpa/inet.h>
#include <errno.h>
#include <fcntl.h>
#include <string.h>

#define debug_log(_fmt_, ...) \
    printf("\033[0;33m[%s:%d]\033[0m " _fmt_, __FILE__, __LINE__, ## __VA_ARGS__)

extern int
new_accept_fd(int fd);

extern int
read_from_client(int fd);

extern int
make_socket(uint16_t port);

extern int
set_non_block(int fd);

#endif
