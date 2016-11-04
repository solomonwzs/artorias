#include "mw_server.h"

#define CONN_TIMEOUT 5


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
      if (events[i].events & EPOLLERR ||
          events[i].events & EPOLLHUP ||
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


static void
worker_process(int channel_fd) {
  size_t fixed_size[] = {8, 12, 16, 24, 32, 48, 64, 128, 256, 384, 512,
    768, 1024};
  as_mem_pool_fixed_t *mem_pool = mem_pool_fixed_new(
      fixed_size, sizeof(fixed_size) / sizeof(fixed_size[0]));

  int epfd = epoll_create(1);
  struct epoll_event listen_event;
  as_rb_conn_t cfd_conn;

  cfd_conn.fd = channel_fd;
  add_wrap_conn_event(&cfd_conn, listen_event, epfd);

  as_rb_conn_pool_t conn_pool = NULL_RB_CONN_POOL;
  int active_cnt;
  int i;
  struct epoll_event events[100];
  int infd;
  struct epoll_event event;
  int n;
  as_rb_conn_t *wc;
  while (1) {
    active_cnt = epoll_wait(epfd, events, 100, 1*1000);
    for (i = 0; i < active_cnt; ++i) {
      wc = (as_rb_conn_t *)events[i].data.ptr;
      if (events[i].events & EPOLLERR || events[i].events & EPOLLHUP ||
          !(events[i].events & EPOLLIN)) {
        debug_perror("epoll_wait");
        if (wc->fd == channel_fd) {
          close(wc->fd);
        } else {
          close_wrap_conn(&conn_pool, wc);
        }
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
    ot.root = rb_conn_remove_timeout_conn(&conn_pool, CONN_TIMEOUT);
    rb_tree_postorder_travel(&ot, recycle_timeout_conn);
  }
  rb_tree_postorder_travel(&conn_pool.ut_tree, recycle_timeout_conn);
  mem_pool_fixed_destroy(mem_pool);
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
