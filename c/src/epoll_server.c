#include "epoll_server.h"

#include <signal.h>
#include <sys/epoll.h>
#include <sys/socket.h>

#include "bytes.h"
#include "lua_bind.h"
#include "lua_output.h"
#include "lua_utils.h"
#include "server.h"

static volatile int keep_running = 1;
static void init_handler(int dummy) {
  keep_running = 0;
}

void epoll_server(int fd) {
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
        epoll_ctl(epfd, EPOLL_CTL_DEL, infd, NULL);

        char buf[1024];
        infd = events[i].data.fd;
        while (1) {
          n = read(infd, buf, 1);
          if (n < 0 && errno == EAGAIN) {
            send(infd, "+OK\r\n", 5, MSG_NOSIGNAL);
            event.data.fd = infd;
            event.events = EPOLLIN | EPOLLET;
            epoll_ctl(epfd, EPOLL_CTL_ADD, infd, &event);
            break;
          } else if (n < 0) {
            debug_log("close(%d): %d\n", errno, infd);
            close(infd);
            break;
          } else if (n == 0) {
            debug_log("close: %d\n", infd);
            close(infd);
            break;
          } else {
            buf[n] = '\0';
            debug_log("<-(%d): %s\n", n, buf);
            // send(infd, "+OK\r\n", 5, MSG_NOSIGNAL);
            event.data.fd = infd;
            event.events = EPOLLIN | EPOLLET;
            epoll_ctl(epfd, EPOLL_CTL_ADD, infd, &event);
          }
        }

        // infd = events[i].data.fd;
        // n = simple_read_from_client(infd);
        // if (n <= 0) {
        //   debug_log("close: %d %d\n", infd, errno);
        //   close(infd);
        // } else {
        //   event.data.fd = infd;
        //   event.events = EPOLLOUT | EPOLLET;
        //   epoll_ctl(epfd, EPOLL_CTL_MOD, infd, &event);
        // }
      } else if (events[i].events & EPOLLOUT) {
        infd = events[i].data.fd;

#define SND_SIZE 10

        char msg[SND_SIZE];
        memset(msg, 'a', SND_SIZE);
        msg[0] = '+';
        msg[SND_SIZE - 2] = '\r';
        msg[SND_SIZE - 1] = '\n';

        n = simple_write_to_client(infd, msg, SND_SIZE);
        debug_log("%d\n", n);
        if (n <= 0) {
          close(infd);
        } else {
          event.data.fd = infd;
          event.events = EPOLLIN | EPOLLET;
          epoll_ctl(epfd, EPOLL_CTL_MOD, infd, &event);
        }
      } else {
        debug_log("%d\n", events[i].events);
      }
    }
  }
}
