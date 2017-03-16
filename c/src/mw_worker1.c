#include "mw_worker.h"
#include "mem_pool.h"
#include <sys/epoll.h>
#include "server.h"


void
worker_process1(int channel_fd, as_lua_pconf_t *cnf) {
  as_cnf_return_t ret;
  int conn_timeout;

  ret = lpconf_get_pconf_value(cnf, 1, "conn_timeout");
  conn_timeout = ret.val.i;

  size_t fixed_size[] = {8, 12, 16, 24, 32, 48, 64, 128, 256, 384, 512,
    768, 1024};
  as_mem_pool_fixed_t *mem_pool = mpf_new(
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
        if (errno != EAGAIN && errno != EWOULDBLOCK) {
          debug_perror("epoll_wait");
        }
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

          as_rb_conn_t *nwc = mpf_alloc(mem_pool, sizeof(as_rb_conn_t));
          rb_conn_init(NULL, nwc, infd);
          rb_conn_pool_insert(&conn_pool, nwc);
          add_wrap_conn_event(nwc, event, epfd);
        }
      } else if (events[i].events & EPOLLIN) {
        n = simple_read_from_client(wc->fd);
        if (n <= 0) {
          close_wrap_conn(&conn_pool, wc);
        } else {
          rb_conn_pool_update_conn_ut(&conn_pool, wc);
          write(wc->fd, "+OK\r\n", 5);
        }
      }
    }
    as_rb_tree_t ot;
    ot.root = rb_conn_remove_timeout_conn(&conn_pool, conn_timeout);
    rb_tree_postorder_travel(&ot, recycle_conn);
  }
  rb_tree_postorder_travel(&conn_pool.ut_tree, recycle_conn);
  mpf_destroy(mem_pool);
}
