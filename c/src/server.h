#ifndef __SERVER_H__
#define __SERVER_H__

#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <arpa/inet.h>
#include <errno.h>
#include <fcntl.h>
#include <string.h>
#include "utils.h"

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
