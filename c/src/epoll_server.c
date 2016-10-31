#include "epoll_server.h"
#include "mem_pool.h"
#include "wrap_conn.h"

#define as_malloc malloc
#define as_free free

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

  listen_event.events = EPOLLIN | EPOLLET;
  wfd.fd = fd;
  listen_event.data.ptr = &wfd;
  set_non_block(fd);
  epoll_ctl(epfd, EPOLL_CTL_ADD, fd, &listen_event);

  as_rb_conn_pool_t conn_pool;
  conn_pool.ut_tree.root = NULL;
  conn_pool.fd_tree.root = NULL;

  int active_cnt;
  int i;
  struct epoll_event events[100];
  int infd;
  struct epoll_event event;
  int n;
  as_rb_conn_t *wc;
  while (1) {
    active_cnt = epoll_wait(epfd, events, 100, -1);
    for (i = 0; i < active_cnt; ++i) {
      wc = (as_rb_conn_t *)events[i].data.ptr;
      if (events[i].events & EPOLLERR || events[i].events & EPOLLHUP ||
          !(events[i].events & EPOLLIN)) {
        debug_perror("epoll_wait");
        close(wc->fd);
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

          set_non_block(infd);
          event.data.ptr = nwc;
          event.events = EPOLLIN | EPOLLET;
          epoll_ctl(epfd, EPOLL_CTL_ADD, infd, &event);
        }
      } else if (events[i].events & EPOLLIN) {
        rb_conn_pool_update_conn_ut(&conn_pool, wc);

        infd = wc->fd;
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

  child_sockets = (int *)as_malloc(sizeof(int) * n);
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

  as_free(child_sockets);
  close(fd);
}

static void
worker_process(int channel_fd) {
  size_t fixed_size[] = {8, 12, 16, 24, 32, 48, 64, 128, 256, 384, 512,
    768, 1024};
  as_mem_pool_fixed_t *mem_pool = mem_pool_fixed_new(
      fixed_size, sizeof(fixed_size) / sizeof(fixed_size[0]));

  int epfd = epoll_create(1);
  struct epoll_event listen_event;
  as_rb_conn_t cfd_conn;

  listen_event.events = EPOLLIN | EPOLLET;
  cfd_conn.fd = channel_fd;
  listen_event.data.ptr = &cfd_conn;
  set_non_block(channel_fd);
  epoll_ctl(epfd, EPOLL_CTL_ADD, channel_fd, &listen_event);

  as_rb_conn_pool_t conn_pool;
  conn_pool.ut_tree.root = NULL;
  conn_pool.fd_tree.root = NULL;

  int active_cnt;
  int i;
  struct epoll_event events[100];
  int infd;
  struct epoll_event event;
  int n;
  as_rb_conn_t *wc;
  while (1) {
    active_cnt = epoll_wait(epfd, events, 100, -1);
    for (i = 0; i < active_cnt; ++i) {
      wc = (as_rb_conn_t *)events[i].data.ptr;
      if (events[i].events & EPOLLERR || events[i].events & EPOLLHUP ||
          !(events[i].events & EPOLLIN)) {
        debug_perror("epoll_wait");
        debug_log("%d\n", wc->fd);
        close(wc->fd);
      } else if (wc->fd == channel_fd) {
        while (1) {
          infd = recv_fd_from_socket(channel_fd);
          if (infd == -1) {
            break;
          }

          as_rb_conn_t *nwc = mem_pool_fixed_alloc(
              mem_pool, sizeof(as_rb_conn_t));
          rb_conn_init(nwc, infd);
          rb_conn_pool_insert(&conn_pool, nwc);

          set_non_block(infd);
          event.data.ptr = nwc;
          event.events = EPOLLIN | EPOLLET;
          epoll_ctl(epfd, EPOLL_CTL_ADD, infd, &event);
        }
      } else if (events[i].events & EPOLLIN) {
        rb_conn_pool_update_conn_ut(&conn_pool, wc);

        infd = wc->fd;
        n = read_from_client(infd);
        if (n <= 0) {
          close(infd);
        } else {
          write(infd, "+OK\r\n", 5);
        }
      }
    }
  }

  mem_pool_fixed_destroy(mem_pool);
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
