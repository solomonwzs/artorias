#include "epoll_server.h"


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
    active_cnt = epoll_wait(epfd, events, 100, 1000);
    debug_log("%d\n", active_cnt);
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
epoll_server2(int fd) {
  size_t fixed_size[] = {8, 12, 16, 24, 32, 48, 64, 128, 256, 384, 512,
    768, 1024};
  as_mem_pool_fixed_t *mem_pool = mem_pool_fixed_new(
      fixed_size, sizeof(fixed_size) / sizeof(fixed_size[0]));

  int epfd = epoll_create(1);
  struct epoll_event listen_event;
  as_rb_conn_t wfd;
  wfd.fd = fd;
  add_wrap_conn_event(&wfd, listen_event, epfd);

  as_rb_conn_pool_t conn_pool = empty_rb_conn_pool;
  int active_cnt;
  int i;
  struct epoll_event events[100];
  int infd;
  struct epoll_event event;
  int n;
  as_rb_conn_t *wc;
  while (1) {
    active_cnt = epoll_wait(epfd, events, 100, 5*1000);
    for (i = 0; i < active_cnt; ++i) {
      wc = (as_rb_conn_t *)events[i].data.ptr;
      if (events[i].events & EPOLLERR ||
          events[i].events & EPOLLHUP ||
          !(events[i].events & EPOLLIN)) {
        debug_perror("epoll_wait");
        if (wc->fd == fd) {
          close(wc->fd);
        } else {
          close_wrap_conn(&conn_pool, wc);
        }
      } else if (wc->fd == fd) {
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

          as_rb_conn_t *nwc = mem_pool_fixed_alloc(
              mem_pool, sizeof(as_rb_conn_t));
          rb_conn_init(nwc, infd);
          rb_conn_pool_insert(&conn_pool, nwc);
          add_wrap_conn_event(nwc, event, epfd);
        }
      } else if (events[i].events & EPOLLIN) {
        n = read_from_client(wc->fd);
        if (n <= 0) {
          close_wrap_conn(&conn_pool, wc);
        } else {
          rb_conn_pool_update_conn_ut(&conn_pool, wc);
          write(wc->fd, "+OK\r\n", 5);
        }
      }
    }
    as_rb_tree_t ot;
    ot.root = rb_conn_remove_timeout_conn(&conn_pool, 5);
    rb_tree_postorder_travel(&ot, recycle_timeout_conn);
  }
  rb_tree_postorder_travel(&conn_pool.ut_tree, recycle_timeout_conn);
  mem_pool_fixed_destroy(mem_pool);
}
