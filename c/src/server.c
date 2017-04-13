#include <arpa/inet.h>
#include <fcntl.h>
#include "utils.h"
#include "server.h"

#define MAXLEN  256


static inline int
simple_bind_name_to_socket(int sock, uint16_t port) {
  struct sockaddr_in addr;
  bzero(&addr, sizeof(addr));
  addr.sin_family = AF_INET;
  addr.sin_addr.s_addr = htonl(INADDR_ANY);
  addr.sin_port = htons(port);
  return bind(sock, (struct sockaddr *)&addr, sizeof(addr));
}


static inline int
simple_connect_socket(int sock, const char *hostname, uint16_t port) {
  struct sockaddr_in addr;
  bzero(&addr, sizeof(addr));
  addr.sin_family = AF_INET;
  if (inet_aton(hostname, &addr.sin_addr) == 0) {
    return -1;
  }
  addr.sin_port = htons(port);
  return connect(sock, (struct sockaddr *)&addr, sizeof(addr));
}


int
set_non_block(int fd) {
  int flags;
  if ((flags = fcntl(fd, F_GETFL, 0)) == -1) {
    debug_perror("non-block");
    return -1;
  }
  if (fcntl(fd, F_SETFL, flags|O_NONBLOCK) == -1){
    debug_perror("non-block");
    return -1;
  }
  return 0;
}


int
make_server_socket(uint16_t port) {
  int sock = socket(AF_INET, SOCK_STREAM, 0);
  if (sock < 0) {
    debug_perror("socket");
    return -1;
  }

  int optval = 1;
  setsockopt(sock, SOL_SOCKET, SO_REUSEADDR, (const void*)&optval,
             sizeof(int));

  if (simple_bind_name_to_socket(sock, port) < 0) {
    close(sock);
    debug_perror("bind");
    return -1;
  }

  return sock;
}


int
make_client_socket(const char *hostname, uint16_t port) {
  int sock = socket(AF_INET, SOCK_STREAM, 0);
  if (sock < 0) {
    debug_perror("socket");
    return -1;
  }

  if (simple_bind_name_to_socket(sock, 0) < 0) {
    close(sock);
    debug_perror("bind");
    return -1;
  }

  if (simple_connect_socket(sock, hostname, port) < 0) {
    close(sock);
    debug_perror("connect");
    return -1;
  }

  return sock;
}


int
simple_read_from_client(int fd) {
  char buffer[MAXLEN + 1];
  int nbyte;

  int n = 0;
  do {
    nbyte = read(fd, buffer, MAXLEN);
    if (nbyte < 0) {
      if (errno == EAGAIN) {
        return n;
      } else {
        debug_perror("read");
        return -1;
      }
    } else if (nbyte == 0) {
      return 0;
    } else {
      // buffer[nbyte] = '\0';
      // debug_log("Server: got message: len: %d, '%s'\n", nbyte, buffer);
      n += nbyte;
    }
  } while (nbyte > 0);
  return 0;
  // return read(fd, buffer, MAXLEN);
}


int
simple_write_to_client(int fd, const char *buf, size_t len) {
  int i = 0;
  while (i < len) {
    int n = send(fd, buf + i, len - i, MSG_NOSIGNAL);
    if (n < 0) {
      if (errno == EAGAIN) {
        return i;
      } else {
        debug_perror("send");
        return -1;
      }
    } else if (n == 0) {
      return i;
    } else {
      i += n;
    }
  }
  return len;
}


int
new_accept_fd(int fd) {
  int infd;
  struct sockaddr_in in_addr;
  unsigned int size;

  size = sizeof(in_addr);
  infd = accept(fd, (struct sockaddr *)&in_addr, &size);
  if (infd >= 0) {
    debug_log("Server: connect from host %s, port %d.\n",
              inet_ntoa(in_addr.sin_addr),
              ntohs(in_addr.sin_port));
  }
  return infd;
}


int
send_fd_by_socket(int socket, int fd) {
  struct iovec io;
  struct msghdr msg = {0};
  char buf[CMSG_SPACE(sizeof(fd))];
  char mbuf[256];

  io.iov_base = mbuf;
  io.iov_len = sizeof(mbuf);

  msg.msg_iov = &io;
  msg.msg_iovlen = 1;
  msg.msg_control = buf;
  msg.msg_controllen = sizeof(buf);

  struct cmsghdr *cmsg = CMSG_FIRSTHDR(&msg);
  cmsg->cmsg_level = SOL_SOCKET;
  cmsg->cmsg_type = SCM_RIGHTS;
  cmsg->cmsg_len = CMSG_LEN(sizeof(fd));

  *((int *)CMSG_DATA(cmsg)) = fd;

  if (sendmsg(socket, &msg, 0) < 0) {
    debug_perror("send_fd");
    return -1;
  }
  return 0;
}

int
recv_fd_from_socket(int socket) {
  struct msghdr msg = {0};
  char mbuf[256];
  char cbuf[256];
  struct iovec io;

  io.iov_base = mbuf;
  io.iov_len = sizeof(mbuf);

  msg.msg_iov = &io;
  msg.msg_iovlen = 1;
  msg.msg_control = cbuf;
  msg.msg_controllen = sizeof(cbuf);

  if (recvmsg(socket, &msg, 0) < 0) {
    if (errno != EAGAIN && errno != EWOULDBLOCK) {
      debug_perror("recv_fd");
    }
    return -1;
  }

  struct cmsghdr *cmsg = CMSG_FIRSTHDR(&msg);
  unsigned char *data = CMSG_DATA(cmsg);
  int fd = *((int *)data);

  return fd;
}


int
set_socket_send_buffer_size(int fd, int snd_size) {
  int sendbuff = snd_size;
  if (setsockopt(fd, SOL_SOCKET, SO_SNDBUF, &sendbuff,
                 sizeof(sendbuff)) == -1) {
    debug_perror("setsockopt");
    return -1;
  }
  socklen_t optlen = sizeof(sendbuff);
  getsockopt(fd, SOL_SOCKET, SO_SNDBUF, &sendbuff, &optlen);
  return sendbuff;
}
