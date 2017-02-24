#ifndef __SERVER_H__
#define __SERVER_H__

#include <stdlib.h>

extern int
new_accept_fd(int fd);

extern int
simple_read_from_client(int fd);

extern int
simple_write_to_client(int fd, const char *buf, size_t len);

extern int
make_socket(unsigned port);

extern int
set_non_block(int fd);

extern int
send_fd_by_socket(int socket, int fd);

extern int
recv_fd_from_socket(int socket);

extern int
set_socket_send_buffer_size(int fd, int size);


#endif
