#include <unistd.h>
#include <sys/socket.h>
#include "server.h"
#include "channel.h"
#include "utils.h"


void
t() {
  int child;
  int sockets[2];
  char buf[1024];
  if (socketpair(AF_UNIX, SOCK_STREAM, 0, sockets) < 0) {
    debug_perror("socketpair");
    exit(1);
  }

  if ((child = fork()) == -1) {
    debug_perror("fork");
  } else if (child) {
    close(sockets[0]);

    if (read(sockets[1], buf, 1024) < 0) {
      debug_perror("read_stream");
    }
    debug_log("--> %s\n", buf);

    if (write(sockets[1], "world", 5) < 0) {
      debug_perror("write_stream");
    }

    close(sockets[1]);
  } else {
    close(sockets[1]);

    if (write(sockets[0], "hello", 5) < 0) {
      debug_perror("write_stream");
    }
    if (read(sockets[0], buf, 1024) < 0) {
      debug_perror("read_stream");
    }
    debug_log("--> %s\n", buf);

    close(sockets[0]);
  }
}
