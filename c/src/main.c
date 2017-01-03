#include "server.h"
#include "rb_tree.h"
#include "mem_buddy.h"
#include "mem_slot.h"
#include "channel.h"
#include "mem_pool.h"
#include "bytes.h"
#include "select_server.h"
#include "mw_server.h"
#include "lua_pconf.h"
#include "epoll_server.h"
#include <unistd.h>
#include <sys/socket.h>

#define PORT 5555
#define DEFAULT_CONF_FILE "conf/example.lua"


void
bytes_test() {
  size_t s[] = {8, 12, 16, 24, 32, 48, 64, 128, 256};
  as_mem_pool_fixed_t *p = mpf_new(s, sizeof(s) / sizeof(s[0]));

  as_bytes_t buf;
  bytes_init(&buf, p);
  bytes_append(&buf, "1234\0", 4);
  bytes_append(&buf, "abcde\0", 5);
  bytes_reset_used(&buf, 3);
  bytes_append(&buf, "ABCDEFGH\0", 8);
  bytes_append(&buf, "ABCDEFGH\0", 8);
  bytes_print(&buf);
  bytes_reset_used(&buf, 11);
  bytes_append(&buf, "1234\0", 4);
  bytes_print(&buf);

  bytes_destroy(&buf);
  mpf_destroy(p);
}


void
server_test(as_lua_pconf_t *cnf) {
  as_cnf_return_t ret = lpconf_get_pconf_value(cnf, 1, "tcp_port");

  int sock;
  sock = make_socket(ret.val.i);
  set_non_block(sock);
  if (listen(sock, 500) < 0) {
    perror("listen");
    exit(EXIT_FAILURE);
  }
  // select_server(sock);
  epoll_server2(sock);
  // master_workers_server(sock, 2);
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
  debug_log("%zu\n", sizeof(as_mem_data_fixed_t));

  size_t s[] = {8, 12, 16, 24, 32, 48, 64, 128, 256};
  as_mem_pool_fixed_t *p = mpf_new(s, sizeof(s) / sizeof(s[0]));

  void *a = mpf_alloc(p, 18);
  debug_log("%zu\n", mpf_data_size(a));
  debug_log("%p\n", a);
  debug_log("%d\n", p->used);
  debug_log("%d\n", p->empty);
  void *b = mpf_alloc(p, 18);
  debug_log("%p\n", b);
  mpf_recycle(b);
  mpf_recycle(a);

  mpf_destroy(p);
}

void
lua_pconf_test() {
  as_lua_pconf_t *cnf = lpconf_new("conf/example.lua");
  as_cnf_return_t ret = lpconf_get_pconf_value(cnf, 2, "server_no", "2");
  debug_log("%d %d\n", ret.type, ret.val.i);
  ret = lpconf_get_pconf_value(cnf, 1, "name");
  debug_log("%d %s\n", ret.type, ret.val.s);
  lpconf_destroy(cnf);
}


int
main(int argc, char **argv) {
  as_lua_pconf_t *cnf = NULL;
  if (argc >= 2) {
    cnf = lpconf_new(argv[1]);
  } else {
      cnf = lpconf_new(DEFAULT_CONF_FILE);
  }
  if (cnf == NULL) {
    exit(1);
  }

  // lua_pconf_test();
  // mem_pool_test();
  server_test(cnf);
  // rb_tree_test();
  // bytes_test();

  lpconf_destroy(cnf);
  return 0;
}
