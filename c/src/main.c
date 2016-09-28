#include "epoll_server.h"
#include "select_server.h"
#include <unistd.h>

#define PORT 5555

int
main(int argc, char **argv) {
  int sock;
  sock = make_socket(PORT);
  set_non_block(sock);
  if (listen(sock, 500) < 0) {
    perror("listen");
    exit(EXIT_FAILURE);
  }
  // select_server(sock);
  epoll_server(sock);

  return 0;
}
