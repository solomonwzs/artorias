#include "wrap_event.h"

int static inline
set_non_block(int fd) {
  int flags;
  if ((flags = fcntl(fd, F_GETFL)) == -1) {
    return -1;
  }
  if (fcntl(fd, F_SETFL, flags|O_NONBLOCK) == -1){
    return -1;
  }
  return 0;
}

int
make_socket(uint16_t port) {
  int sock;
  struct sockaddr_in name;

  sock = socket(AF_INET, SOCK_STREAM, 0);
  if (sock < 0) {
    perror("socket");
    exit(EXIT_FAILURE);
  }

  int optval = 1;
  setsockopt(sock, SOL_SOCKET, SO_REUSEADDR, (const void*)&optval,
             sizeof(int));

  if (set_non_block(sock) == -1) {
    perror("no-block socket");
    exit(EXIT_FAILURE);
  }

  bzero((char *)&name, sizeof(name));
  name.sin_family = AF_INET;
  name.sin_port = htons(port);
  name.sin_addr.s_addr = htonl(INADDR_ANY);
  if (bind(sock, (struct sockaddr *)&name, sizeof(name)) < 0) {
    close(sock);
    perror("bind");
    exit(EXIT_FAILURE);
  }

  return sock;
}

void
event_server(int fd) {
}
