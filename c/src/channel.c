#include "channel.h"

void
t() {
  int child;
  int sockets[2];
  char buf[1024];
  if (socketpair(AF_UNIX, SOCK_STREAM, 0, sockets) < 0) {
    debug_perror("socketpair");
    exit(1);
  }
  debug_log("%d %d\n", sockets[0], sockets[1]);

  // child = fork();
  // if (child == -1) {
  //   debug_perror("fork");
  // } else if (child) {
  // }
}
