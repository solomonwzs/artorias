#include "server.h"
#include "mem_pool.h"
#include <sys/epoll.h>
#include <sys/socket.h>
#include "mw_server.h"
#include "mw_worker.h"


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


void
master_workers_server(as_lua_pconf_t *cnf) {
  int child;
  int sockets[2];
  int *child_sockets;
  int channel_fd;
  int port;
  int n;
  int fd;
  as_cnf_return_t ret;

  ret = lpconf_get_pconf_value(cnf, 1, "tcp_port");
  port = ret.val.i;

  ret = lpconf_get_pconf_value(cnf, 1, "n_workers");
  n = ret.val.i;

  fd = make_server_socket(port);
  if (fd < 0) {
    exit(EXIT_FAILURE);
  }
  set_non_block(fd);
  if (listen(fd, 500) < 0) {
    perror("listen");
    exit(EXIT_FAILURE);
  }

  child_sockets = (int *)as_malloc(sizeof(int) * n);
  int i;
  int m = 0;

  void (*wp)(int, as_lua_pconf_t *);
  ret = lpconf_get_pconf_value(cnf, 1, "worker_type");
  int worker_type = ret.val.i;
  switch (worker_type) {
    default:
      wp = worker_process;
  }

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
    // for (i = 0; i < m; ++i) {
    //   close(child_sockets[i]);
    // }
    wp(channel_fd, cnf);
  }

  as_free(child_sockets);
  close(fd);
}
