#include "server.h"
#include "wrap_event.h"

#define PORT    5555
#define MAXLEN  5

int static inline
read_from_client(int fd) {
  char buffer[MAXLEN];
  int nbytes;

  nbytes = read(fd, buffer, MAXLEN);
  if (nbytes < 0) {
    perror("read");
    return -1 ? errno != EAGAIN : 0;
  } else if (nbytes == 0) {
    return -1;
  } else {
    debug_log("Server: got message: '%s'\n", buffer);
    return nbytes;
  }
}

int static inline
new_accept_fd(int fd) {
  int infd;
  struct sockaddr_in clientname;
  unsigned int size;

  size = sizeof(clientname);
  infd = accept(fd, (struct sockaddr *)&clientname, &size);
  if (infd >= 0) {
    debug_log("Server: connect from host %s, port %d.\n",
              inet_ntoa(clientname.sin_addr),
              ntohs(clientname.sin_port));
  }
  return infd;
}

void static inline
select_server(int fd) {
  int i;
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
          int infd = new_accept_fd(fd);
          if (infd < 0) {
            perror("accept");
            continue;
          }
          set_non_block(infd);
          FD_SET(infd, &active_fd_set);
        }
        else {
          int n;
          do {
            n = read_from_client(i);
            if (n == -1) {
              close(i);
              break;
            }
          } while (n > 0);
          write(i, "+OK\r\n", 5);
        }
      }
    }
  }
}

void static inline
epoll_server(int fd) {
  int epfd = epoll_create1(0);
  struct epoll_event listen_event;

  listen_event.events = EPOLLIN | EPOLLET;
  listen_event.data.fd = fd;
  epoll_ctl(epfd, EPOLL_CTL_ADD, fd, &listen_event);

  struct epoll_event events[10];
  int active_cnt;
  int i;
  while (1) {
    active_cnt = epoll_wait(epfd, events, 10, -1);
    for (i = 0; i < active_cnt; ++i) {
      if (events[i].events & EPOLLERR || events[i].events & EPOLLHUP ||
          !(events[i].events & EPOLLIN)) {
        perror("epoll_wait");
        continue;
      } else if (events[i].data.fd == fd) {
        int infd = new_accept_fd(fd);
        if (infd < 0) {
          perror("accept");
          continue;
        }
        set_non_block(infd);
        
        struct epoll_event event;
        event.data.fd = infd;
        event.events = EPOLLIN | EPOLLET;
        epoll_ctl(epfd, EPOLL_CTL_ADD, infd, &event);
      } else if (events[i].events & EPOLLIN) {
        int n;
        do {
          n = read_from_client(events[i].data.fd);
          if (n == -1) {
            close(events[i].data.fd);
            break;
          }
        } while (n > 0);
        write(events[i].data.fd, "+OK\r\n", 5);
      }
    }
  }
}

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
