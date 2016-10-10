#include "epoll_server.h"
#include "select_server.h"
#include "channel.h"
#include "mem_buddy.h"
#include "mem_slot.h"
#include <unistd.h>

#define PORT 5555

int
main(int argc, char **argv) {
  // int sock;
  // sock = make_socket(PORT);
  // set_non_block(sock);
  // if (listen(sock, 500) < 0) {
  //   perror("listen");
  //   exit(EXIT_FAILURE);
  // }
  // // select_server(sock);
  // epoll_server(sock);

  printf("%d\n", bits_cl0_u32(12));

  as_buddy_t *b = buddy_new(4);
  unsigned x = buddy_alloc(b, 4);
  unsigned y = buddy_alloc(b, 8);
  unsigned z = buddy_alloc(b, 2);
  buddy_free(b, x);
  buddy_print(b);
  buddy_destroy(b);

  return 0;
}
