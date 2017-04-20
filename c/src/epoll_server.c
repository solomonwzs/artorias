#include "server.h"
#include "bytes.h"
#include "wrap_conn.h"
#include "lua_bind.h"
#include "lua_output.h"
#include "lua_utils.h"
#include <sys/epoll.h>
#include <sys/socket.h>
#include "epoll_server.h"
#include <signal.h>


#define SOCKET_LUA_FILE "luas/t_socket.lua"

#define close_wrap_conn(_wc_) do {\
  debug_log("close: %d\n", _wc_->fd);\
  close(_wc_->fd);\
  rb_conn_pool_delete(_wc_);\
  mpf_recycle(_wc_);\
} while (0)

#define add_wrap_conn_event(_wc_, _e_, _epfd_) do {\
  set_non_block((_wc_)->fd);\
  (_e_).data.ptr = (_wc_);\
  (_e_).events = EPOLLIN | EPOLLET;\
  epoll_ctl((_epfd_), EPOLL_CTL_ADD, (_wc_)->fd, &(_e_));\
} while (0)

#define recycle_conn(_n_) do {\
  as_rb_conn_t *__wc = container_of(_n_, as_rb_conn_t, ut_idx);\
  debug_log("close: %d\n", __wc->fd);\
  close(__wc->fd);\
  mpf_recycle(__wc);\
} while (0)


static volatile int keep_running = 1;
static void
init_handler(int dummy) {
  keep_running = 0;
}


// static void
// process_in_data(as_rb_conn_t *wc, as_rb_conn_pool_t *conn_pool,
//                 as_mem_pool_fixed_t *mem_pool, lua_State *L) {
//   as_bytes_t buf;
//   bytes_init(&buf, mem_pool);
//   int n = bytes_read_from_fd(&buf, wc->fd);
//   // int n = simple_read_from_client(wc->fd);
//   if (n <= 0) {
//     close_wrap_conn(L, conn_pool, wc);
//   } else {
//     rb_conn_pool_update_conn_ut(conn_pool, wc);
//     send(wc->fd, "+OK\r\n", 5, MSG_NOSIGNAL);
//
//     // loutput_redis_ok(L, wc->fd);
//
//     // as_bytes_t buf = NULL_AS_BYTES;
//     // bytes_append(&buf, "1234", 4, mem_pool);
//     // bytes_append(&buf, "abcde", 5, mem_pool);
//     // bytes_write_to_fd(wc->fd, &buf, buf.used);
//     // bytes_destroy(&buf);
//   }
//   // bytes_print(&buf);
//   bytes_destroy(&buf);
// }


static void
process_in_data(as_rb_conn_t *wc, as_rb_conn_pool_t *conn_pool,
                as_mem_pool_fixed_t *mem_pool, lua_State *L) {
  int n = lua_gettop(wc->T);
  lbind_get_lcode_chunk(wc->T, SOCKET_LUA_FILE);
  int ret = lua_pcall(wc->T, 0, LUA_MULTRET, 0);
  if (ret != LUA_OK){
    lb_pop_error_msg(wc->T);
    close_wrap_conn(wc);
  } else if (lua_gettop(wc->T) != n && lua_tointeger(wc->T, -1) == -1) {
    close_wrap_conn(wc);
  } else {
    lua_settop(wc->T, 0);
    rb_conn_update_ut(wc);
  }
}


void
epoll_server(int fd) {
  signal(SIGINT, init_handler);

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
  while (keep_running) {
    active_cnt = epoll_wait(epfd, events, 100, 1000);
    for (i = 0; i < active_cnt; ++i) {
      if (events[i].events & EPOLLERR || events[i].events & EPOLLHUP ||
          !(events[i].events & EPOLLIN || events[i].events & EPOLLOUT)) {
        if (errno != EAGAIN && errno != EWOULDBLOCK) {
          debug_perror("epoll_wait");
        }
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
          set_socket_send_buffer_size(infd, 2048);

          event.data.fd = infd;
          event.events = EPOLLIN | EPOLLET;
          epoll_ctl(epfd, EPOLL_CTL_ADD, infd, &event);
        }
      } else if (events[i].events & EPOLLIN) {
        infd = events[i].data.fd;
        n = simple_read_from_client(infd);
        if (n <= 0) {
          debug_log("close: %d\n", infd);
          close(infd);
        } else {
          event.data.fd = infd;
          event.events = EPOLLOUT | EPOLLET;
          epoll_ctl(epfd, EPOLL_CTL_MOD, infd, &event);
        }
      } else if (events[i].events & EPOLLOUT) {
        infd = events[i].data.fd;
        char msg[5000];
        memset(msg, 'a', 5000);
        msg[0] = '+';
        msg[4997] = '-';
        msg[4998] = '\r';
        msg[4999] = '\n';

        n = simple_write_to_client(infd, msg, 5000);
        debug_log("%d\n", n);
        if (n <= 0) {
          close(infd);
        } else {
          event.data.fd = infd;
          event.events = EPOLLIN | EPOLLET;
          epoll_ctl(epfd, EPOLL_CTL_MOD, infd, &event);
        }
      } else{
        debug_log("%d\n", events[i].events);
      }
    }
  }
}


void
epoll_server2(int fd) {
  signal(SIGINT, init_handler);

  size_t fixed_size[] = DEFAULT_FIXED_SIZE;
  as_mem_pool_fixed_t *mem_pool = mpf_new(
      fixed_size, sizeof(fixed_size) / sizeof(fixed_size[0]));

  int epfd = epoll_create(1);
  struct epoll_event listen_event;
  as_rb_conn_t wfd;
  wfd.fd = fd;
  add_wrap_conn_event(&wfd, listen_event, epfd);

  lua_State *L = lbind_new_state(mem_pool);
  lbind_init_state(L, mem_pool);
  lbind_append_lua_cpath(L, "./bin/?.so");
  lbind_ref_lcode_chunk(L, SOCKET_LUA_FILE);

  as_rb_conn_pool_t conn_pool = NULL_RB_CONN_POOL;
  int active_cnt;
  int i;
  struct epoll_event events[100];
  int infd;
  struct epoll_event event;
  as_rb_conn_t *wc;
  while (keep_running) {
    active_cnt = epoll_wait(epfd, events, 100, 5*1000);
    for (i = 0; i < active_cnt; ++i) {
      wc = (as_rb_conn_t *)events[i].data.ptr;
      if (events[i].events & EPOLLERR ||
          events[i].events & EPOLLHUP ||
          !(events[i].events & EPOLLIN || events[i].events & EPOLLOUT)) {
        if (errno != EAGAIN && errno != EWOULDBLOCK) {
          debug_perror("epoll_wait");
        }
        if (wc->fd == fd) {
          close(wc->fd);
        } else {
          close_wrap_conn(wc);
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

          as_rb_conn_t *nwc = mpf_alloc(
              mem_pool, sizeof(as_rb_conn_t));
          rb_conn_init(nwc, infd);
          nwc->T = lbind_new_fd_lthread(L, infd);
          rb_conn_pool_insert(&conn_pool, nwc);
          add_wrap_conn_event(nwc, event, epfd);
        }
      } else if (events[i].events & EPOLLIN) {
        process_in_data(wc, &conn_pool, mem_pool, L);
      } else if (events[i].events & EPOLLOUT) {
      }
    }
    as_rb_tree_t ot;
    ot.root = rb_conn_remove_timeout_conn(&conn_pool, 120);
    rb_tree_postorder_travel(&ot, recycle_conn);
  }
  lua_close(L);
  rb_tree_postorder_travel(&conn_pool.ut_tree, recycle_conn);
  mpf_destroy(mem_pool);
}
