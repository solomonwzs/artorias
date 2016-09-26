#include "epoll_server.h"

void
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

static void
event_set(struct myevent_s *ev, int fd, void *arg,
          void (*call_back)(int, int, void*)) {
  ev->fd = fd;
  ev->call_back = call_back;
  ev->events = 0;
  ev->arg = arg;
  ev->status = 0;
  ev->last_active = time(NULL);
}

static void
event_add(struct myevent_s *ev, int fd, int events) {
  struct epoll_event epv = {0, {0}};
  int op;

  epv.data.ptr = ev;
  epv.events = ev->events = events;
  if (ev->status == 1) {
    op = EPOLL_CTL_MOD;
  } else {
    op = EPOLL_CTL_ADD;
    ev->status = 1;
  }

  if (epoll_ctl(fd, op, ev->fd, &epv) < 0) {
    perror("epoll_ctl");
  }
}

static void
event_del(struct myevent_s *ev, int fd) {
  struct epoll_event epv = {0, {0}};

  if (ev->status != 1) {
    return;
  }

  epv.data.ptr = ev;
  ev->status = 0;
  epoll_ctl(fd, EPOLL_CTL_DEL, ev->fd, &epv);
}
