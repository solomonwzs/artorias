#include "epoll_server.h"
#include "select_server.h"
#include "channel.h"
#include "mem_buddy.h"
#include "mem_slot.h"
#include "mem_pool.h"
#include <unistd.h>

#define PORT 5555


void
server_test() {
  int sock;
  sock = make_socket(PORT);
  set_non_block(sock);
  if (listen(sock, 500) < 0) {
    perror("listen");
    exit(EXIT_FAILURE);
  }
  // select_server(sock);
  epoll_server(sock);
}


void
mem_buddy_test() {
  as_mem_buddy_t *b = buddy_new(4);
  unsigned x = buddy_alloc(b, 4);
  buddy_alloc(b, 8);
  buddy_alloc(b, 2);
  buddy_free(b, x);
  buddy_print(b);
  buddy_destroy(b);
}


void
mem_slot_test() {
  as_mem_slot_t *s = slot_new(4);
  slot_alloc(s, 4);
  int a = slot_alloc(s, 5);
  slot_alloc(s, 3);
  slot_free(s, a, 5);
  slot_alloc(s, 6);

  slot_print(s);
  slot_destroy(s);
}


void
mem_pool_test() {
  size_t s[] = {8, 12, 16, 24, 32, 48, 64, 128, 256};
  as_mem_pool_fix_t *p = mem_pool_fix_new(s, sizeof(s) / sizeof(s[0]));

  as_mem_data_fix_t *a = mem_pool_fix_alloc(p, 18);
  printf("%zu\n", a->size);
  printf("%d\n", p->empty);
  mem_pool_fix_recycle(p, a);
  printf("%d\n", p->empty);

  mem_pool_fix_destory(p);
}


int
main(int argc, char **argv) {
  mem_pool_test();
  return 0;
}
