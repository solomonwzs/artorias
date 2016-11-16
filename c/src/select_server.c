#include "select_server.h"
#include <sys/select.h>
#include "server.h"
#include "utils.h"


void
select_server(int fd) {
  int i;
  fd_set active_fd_set, read_fd_set;

  FD_ZERO(&active_fd_set);
  FD_SET(fd, &active_fd_set);
  while (1) {
    read_fd_set = active_fd_set;
    if (select(FD_SETSIZE, &read_fd_set, NULL, NULL, NULL) < 0) {
      debug_perror("select");
      exit(EXIT_FAILURE);
    }

    for (i = 0; i < FD_SETSIZE; ++i) {
      if (FD_ISSET(i, &read_fd_set)) {
        if (i == fd) {
          int infd = new_accept_fd(fd);
          if (infd < 0) {
            debug_perror("accept");
            continue;
          }
          set_non_block(infd);
          FD_SET(infd, &active_fd_set);
        }
        else {
          int n = read_from_client(i);
          if (n <= 0) {
            close(i);
            FD_CLR(i, &active_fd_set);
          } else {
            write(i, "+OK\r\n", 5);
          }
        }
      }
    }
  }
}
