#include "server.h"

#define PORT    5555
#define MAXLEN  1024


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
make_socket(uint16_t port) {
  int sock;
  struct sockaddr_in name;

  sock = socket(AF_INET, SOCK_STREAM, 0);
  if (sock < 0) {
    debug_perror("socket");
    exit(EXIT_FAILURE);
  }

  int optval = 1;
  setsockopt(sock, SOL_SOCKET, SO_REUSEADDR, (const void*)&optval,
             sizeof(int));

  bzero((char *)&name, sizeof(name));
  name.sin_family = AF_INET;
  name.sin_port = htons(port);
  name.sin_addr.s_addr = htonl(INADDR_ANY);
  if (bind(sock, (struct sockaddr *)&name, sizeof(name)) < 0) {
    close(sock);
    debug_perror("bind");
    exit(EXIT_FAILURE);
  }

  return sock;
}


int
read_from_client(int fd) {
  char buffer[MAXLEN + 1];
  int nbytes;

  int n = 0;
  do {
    nbytes = read(fd, buffer, MAXLEN);
    if (nbytes < 0) {
      if (errno == EAGAIN) {
        return n;
      } else {
        debug_perror("read");
        return -1;
      }
    } else if (nbytes == 0) {
      return 0;
    } else {
      buffer[nbytes] = '\0';
      debug_log("Server: got message: len: %d, '%s'\n", nbytes, buffer);
      n += nbytes;
    }
  } while (nbytes > 0);
  return 0;
  // return read(fd, buffer, MAXLEN);
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
    debug_perror("recv_fd");
    return -1;
  }

  struct cmsghdr *cmsg = CMSG_FIRSTHDR(&msg);
  unsigned char *data = CMSG_DATA(cmsg);
  int fd = *((int *)data);

  return fd;
}
