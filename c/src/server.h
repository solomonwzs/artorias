#ifndef __SERVER_H__
#define __SERVER_H__

extern int
new_accept_fd(int fd);

extern int
simple_read_from_client(int fd);

extern int
make_socket(unsigned port);

extern int
set_non_block(int fd);

extern int
send_fd_by_socket(int socket, int fd);

extern int
recv_fd_from_socket(int socket);

#endif
