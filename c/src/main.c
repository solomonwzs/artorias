#include "server.h"
#include "rb_tree.h"
#include "mem_buddy.h"
#include "mem_slot.h"
#include "channel.h"
#include "mem_pool.h"
#include "bytes.h"
#include "select_server.h"
#include "mw_server.h"
#include "mw_worker.h"
#include "lua_utils.h"
#include "lua_bind.h"
#include "lua_pconf.h"
#include "epoll_server.h"
#include <unistd.h>
#include <sys/socket.h>

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
mw_server_test(as_lua_pconf_t *cnf) {
  master_workers_server(cnf);
}


void
client_test() {
  char bytes[1024];
  int sock = make_client_socket("127.0.0.1", 6379);
  int n = write(sock, "*2\r\n$3\r\nGET\r\n$1\r\na\r\n", 20);
  debug_log("%d\n", n);
  n = read(sock, bytes, 1024);
  bytes[n] = '\0';
  debug_log("%s\n", bytes);
  close(sock);
}


void
server_test(as_lua_pconf_t *cnf) {
  as_cnf_return_t ret = lpconf_get_pconf_value(cnf, 1, "tcp_port");
  int sock;
  sock = make_server_socket(ret.val.i);
  if (sock < 0) {
    exit(EXIT_FAILURE);
  }
  set_non_block(sock);
  if (listen(sock, 500) < 0) {
    perror("listen");
    exit(EXIT_FAILURE);
  }
  // select_server(sock);
  // epoll_server(sock);
  test_worker_process2(sock, cnf);
  close(sock);
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


void
luas_test() {
  size_t fixed_size[] = DEFAULT_FIXED_SIZE;
  as_mem_pool_fixed_t *mem_pool = mpf_new(
      fixed_size, sizeof(fixed_size) / sizeof(fixed_size[0]));
  lua_State *L = lbind_new_state(mem_pool);
  lbind_init_state(L, mem_pool);
  lbind_append_lua_cpath(L, "./bin/?.so");

  const char *lfilename = "luas/t_counter.lua";
  lbind_ref_lcode_chunk(L, lfilename);

  // lua_State *T = lua_newthread(L);
  lua_State *T = lbind_new_fd_lthread(L, 12);
  // lbind_free_fd_lthread(L, 1);
  lua_gc(L, LUA_GCCOLLECT, 0);
  if (T == NULL) {
    debug_log("thread null\n");
    return;
  }

  // int ret = luaL_dofile(T, "luas/t_counter.lua");
  // if (ret != LUA_OK) {
  //   lb_pop_error_msg(T);
  // }
  lbind_get_lcode_chunk(T, lfilename);
  debug_log("%d\n", lua_gettop(T));
  lua_pcall(T, 0, LUA_MULTRET, 0);
  debug_log("%d\n", lua_gettop(T));
  debug_log("%lld\n", lua_tointeger(T, -1));

  lua_pushstring(L, "kk");
  lua_pushinteger(L, 11);
  lua_settable(L, LUA_REGISTRYINDEX);

  lua_pushstring(T, "kk");
  lua_gettable(T, LUA_REGISTRYINDEX);
  debug_log("%lld\n", lua_tointeger(T, -1));

  lua_close(L);
  mpf_destroy(mem_pool);
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
  // client_test();
  // mw_server_test(cnf);
  // rb_tree_test();
  // bytes_test();
  // luas_test();

  lpconf_destroy(cnf);
  return 0;
}
