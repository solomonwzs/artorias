#include "epoll_server.h"


void
epoll_server(int fd) {
  int epfd = epoll_create1(0);
  struct epoll_event listen_event;

  listen_event.events = EPOLLIN | EPOLLET;
  listen_event.data.fd = fd;
  epoll_ctl(epfd, EPOLL_CTL_ADD, fd, &listen_event);

  struct epoll_event events[500];
  int active_cnt;
  int i;
  while (1) {
    active_cnt = epoll_wait(epfd, events, 500, -1);
    for (i = 0; i < active_cnt; ++i) {
      if (events[i].events & EPOLLERR || events[i].events & EPOLLHUP ||
          !(events[i].events & EPOLLIN)) {
        debug_perror("epoll_wait");
        continue;
      } else if (events[i].data.fd == fd) {
        int infd = new_accept_fd(fd);
        if (infd < 0) {
          debug_perror("accept");
          continue;
        }
        set_non_block(infd);

        struct epoll_event event;
        event.data.fd = infd;
        event.events = EPOLLIN | EPOLLET;
        epoll_ctl(epfd, EPOLL_CTL_ADD, infd, &event);
      } else if (events[i].events & EPOLLIN) {
        int infd = events[i].data.fd;
        int n = read_from_client(infd);
        if (n <= 0) {
          close(infd);
          break;
        }
        write(infd, "+OK\r\n", 5);
      }
    }
  }
}
