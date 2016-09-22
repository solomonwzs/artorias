#include "server.h"
#include "wrap_event.h"

#define PORT    5555
#define MAXLEN  512

int static inline
read_from_client(int filedes) {
  char buffer[MAXLEN];
  int nbytes;
  nbytes = read(filedes, buffer, MAXLEN);
  if (nbytes < 0) {
    perror("read");
    return -1;
  } else if (nbytes == 0) {
    return -1;
  } else {
    debug_log("Server: %d, got message: '%s'\n", nbytes, buffer);
    write(filedes, "+OK\r\n", 5);
    return 0;
  }
}

void static inline
select_server(int fd) {
  int i;
  unsigned int size;
  struct sockaddr_in clientname;
  fd_set active_fd_set, read_fd_set;

  FD_ZERO(&active_fd_set);
  FD_SET(fd, &active_fd_set);
  while (1) {
    read_fd_set = active_fd_set;
    if (select(FD_SETSIZE, &read_fd_set, NULL, NULL, NULL) < 0) {
      perror("select");
      exit(EXIT_FAILURE);
    }

    for (i = 0; i < FD_SETSIZE; ++i) {
      if (FD_ISSET(i, &read_fd_set)) {
        if (i == fd) {
          int new;
          size = sizeof(clientname);
          new = accept(fd, (struct sockaddr *)&clientname, &size);
          if (new < 0) {
            perror("accept");
            exit(EXIT_FAILURE);
          }
          debug_log("Server: connect from host %s, port %d.\n",
                    inet_ntoa(clientname.sin_addr),
                    ntohs(clientname.sin_port));
          FD_SET (new, &active_fd_set);
        }
        else {
          if (read_from_client(i) < 0) {
            close(i);
            FD_CLR(i, &active_fd_set);
          }
        }
      }
    }
  }
}

int
main(int argc, char **argv) {
  int sock;
  sock = make_socket(PORT);
  if (listen(sock, 500) < 0) {
    perror("listen");
    exit(EXIT_FAILURE);
  }
  select_server(sock);

  return 0;
}
