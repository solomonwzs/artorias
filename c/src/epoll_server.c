#include "epoll_server.h"

#define MALLOC malloc
#define FREE free

static void
worker_process(int channel_fd);

static void
master_process(int socket, int *child_sockets, int n);


void
epoll_server(int fd) {
  int epfd = epoll_create(1);
  struct epoll_event listen_event;

  listen_event.events = EPOLLIN | EPOLLET;
  listen_event.data.fd = fd;
  set_non_block(fd);
  epoll_ctl(epfd, EPOLL_CTL_ADD, fd, &listen_event);

  int active_cnt;
  int i;
  struct epoll_event events[100];
  int infd;
  struct epoll_event event;
  int n;
  while (1) {
    active_cnt = epoll_wait(epfd, events, 100, -1);
    for (i = 0; i < active_cnt; ++i) {
      if (events[i].events & EPOLLERR || events[i].events & EPOLLHUP ||
          !(events[i].events & EPOLLIN)) {
        debug_perror("epoll_wait");
        close(events[i].data.fd);
      } else if (events[i].data.fd == fd) {
        while (1) {
          infd = new_accept_fd(fd);
          if (infd == -1) {
            if (errno == EAGAIN || errno == EWOULDBLOCK) {
              break;
            } else {
              debug_perror("accept");
              break;
            }
          }
          set_non_block(infd);
          event.data.fd = infd;
          event.events = EPOLLIN | EPOLLET;
          epoll_ctl(epfd, EPOLL_CTL_ADD, infd, &event);
        }
      } else if (events[i].events & EPOLLIN) {
        infd = events[i].data.fd;
        n = read_from_client(infd);
        if (n <= 0) {
          close(infd);
        } else {
          write(infd, "+OK\r\n", 5);
        }
      }
    }
  }
}


void
master_workers_server(int fd, int n) {
  int child;
  int sockets[2];
  int *child_sockets;
  int channel_fd;

  child_sockets = (int *)MALLOC(sizeof(int) * n);
  int i;
  int m = 0;
  for (i = 0; i < n; ++i) {
    if (socketpair(AF_UNIX, SOCK_STREAM, 0, sockets) < 0) {
      debug_perror("socketpair");
    }

    m += 1;
    child = fork();
    if (child == -1) {
      debug_perror("fork");
    } else if (child) {
      debug_log("child: %d\n", child);
      close(sockets[0]);
      child_sockets[i] = sockets[1];
    } else {
      close(sockets[1]);
      channel_fd = sockets[0];
      break;
    }
  }

  if (child) {
    master_process(fd, child_sockets, n);
  } else {
    for (i = 0; i < m; ++i) {
      close(child_sockets[i]);
    }
    worker_process(channel_fd);
  }

  FREE(child_sockets);
  close(fd);
}

static void
worker_process(int channel_fd) {
  int epfd = epoll_create(1);
  struct epoll_event listen_event;

  listen_event.events = EPOLLIN | EPOLLET;
  listen_event.data.fd = channel_fd;
  set_non_block(channel_fd);
  epoll_ctl(epfd, EPOLL_CTL_ADD, channel_fd, &listen_event);

  int active_cnt;
  int i;
  struct epoll_event events[100];
  int infd;
  struct epoll_event event;
  int n;
  while (1) {
    active_cnt = epoll_wait(epfd, events, 100, -1);
    for (i = 0; i < active_cnt; ++i) {
      if (events[i].events & EPOLLERR || events[i].events & EPOLLHUP ||
          !(events[i].events & EPOLLIN)) {
        debug_perror("epoll_wait");
        close(events[i].data.fd);
      } else if (events[i].data.fd == channel_fd) {
        while (1) {
          infd = recv_fd_from_socket(channel_fd);
          if (infd == -1) {
            break;
          }
          set_non_block(infd);
          event.data.fd = infd;
          event.events = EPOLLIN | EPOLLET;
          epoll_ctl(epfd, EPOLL_CTL_ADD, infd, &event);
        }
      } else if (events[i].events & EPOLLIN) {
        infd = events[i].data.fd;
        n = read_from_client(infd);
        if (n <= 0) {
          close(infd);
        } else {
          write(infd, "+OK\r\n", 5);
        }
      }
    }
  }
}

static void
master_process(int socket, int *child_sockets, int n) {
  int epfd = epoll_create(1);
  struct epoll_event listen_event;

  listen_event.events = EPOLLIN | EPOLLET;
  listen_event.data.fd = socket;
  set_non_block(socket);
  epoll_ctl(epfd, EPOLL_CTL_ADD, socket, &listen_event);

  int active_cnt;
  int i;
  struct epoll_event events[100];
  int infd;
  int j = 0;
  while (1) {
    active_cnt = epoll_wait(epfd, events, 100, -1);
    for (i = 0; i < active_cnt; ++i) {
      if (events[i].events & EPOLLERR || events[i].events & EPOLLHUP ||
          !(events[i].events & EPOLLIN)) {
        debug_perror("epoll_wait");
        close(events[i].data.fd);
      } else if (events[i].data.fd == socket) {
        while (1) {
          infd = new_accept_fd(socket);
          if (infd == -1) {
            if (errno == EAGAIN || errno == EWOULDBLOCK) {
              break;
            } else {
              debug_perror("accept");
              break;
            }
          }
          send_fd_by_socket(child_sockets[j], infd);
          j = (j + 1) % n;
          close(infd);
        }
      }
    }
  }
}
