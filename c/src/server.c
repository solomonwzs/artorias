#include "server.h"
#include <sys/socket.h>
#include <stdlib.h>
#include <netinet/in.h>

#define PORT    5555
#define MAXLEN  512

int static inline
make_socket(uint16_t port) {
  int sock;
  struct sockaddr_in name;

  sock = socket(PF_INET, SOCK_STREAM, 0);
  if (sock < 0) {
    perror("socket");
    exit(EXIT_FAILURE);
  }

  name.sin_family = AF_INET;
  name.sin_port = htons(port);
  name.sin_addr.s_addr = htonl(INADDR_ANY);
  if (bind(sock, (struct sockaddr *)&name, sizeof(name)) < 0) {
    perror("bind");
    exit(EXIT_FAILURE);
  }

  return sock;
}

int
main(int argc, char **argv) {
  int sock;
  sock = make_socket(PORT);
  return 0;
}
