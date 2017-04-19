#include <sys/epoll.h>
#include "lm_socket.h"
#include "lua_utils.h"
#include "lua_bind.h"
#include "mem_pool.h"
#include "server.h"


// [-3,  +1, e]
static int
lcf_socket_new(lua_State *L) {
  as_lm_socket_t *sock;
  const char *host = luaL_checkstring(L, 1);
  uint16_t port = luaL_checkinteger(L, 2);
  int ot = luaL_checkinteger(L, 3);

  lua_pushstring(L, LRK_SERVER_EPFD);
  lua_gettable(L, LUA_REGISTRYINDEX);
  int epfd = lua_tointeger(L, -1);

  lua_pushstring(L, LRK_MEM_POOL);
  lua_gettable(L, LUA_REGISTRYINDEX);
  as_mem_pool_fixed_t *mp = (as_mem_pool_fixed_t *)lua_touserdata(L, -1);

  sock = (as_lm_socket_t *)lua_newuserdata(L, sizeof(as_lm_socket_t));
  sock->ot_secs = ot;
  sock->conn = NULL;

  luaL_getmetatable(L, LM_SOCKET);
  lua_setmetatable(L, -2);

  sock->conn = (as_rb_conn_t *)mpf_alloc(mp, sizeof(as_rb_conn_t));
  if (sock->conn == NULL) {
    lua_pushstring(L, "alloc error");
    lua_error(L);
  }

  int fd = make_client_socket(host, port);
  if (fd < 0) {
    mpf_recycle(sock->conn);
    lua_pushstring(L, "new socket error");
    lua_error(L);
  }

  rb_conn_init(sock->conn, fd);
  struct epoll_event e;
  set_non_block(fd);
  e.data.ptr = sock->conn;
  e.events = EPOLLIN | EPOLLET;
  if (epoll_ctl(epfd, EPOLL_CTL_ADD, fd, &e) != 0) {
    mpf_recycle(sock->conn);
    close(fd);
    lua_pushstring(L, "epoll error");
    lua_error(L);
  }

  sock->conn->T = L;
  lbind_ref_fd_lthread(L, fd);

  return 1;
}


// [-0, +1, e]
static int
lcf_socket_get_fd(lua_State *L) {
  as_lm_socket_t *sock = (as_lm_socket_t *)luaL_checkudata(
      L, 1, LM_SOCKET);
  lua_pushinteger(L, sock->conn->fd);

  return 1;
}


// [-0, +0, e]
static int
lcf_socket_close(lua_State *L) {
  as_lm_socket_t *sock = (as_lm_socket_t *)luaL_checkudata(
      L, 1, LM_SOCKET);
  if (sock->conn != NULL) {
    close(sock->conn->fd);
    mpf_recycle(sock->conn);
  }

  return 0;
}


// [-0, +0, e]
static int
lcf_socket_destroy(lua_State *L) {
  as_lm_socket_t *sock = (as_lm_socket_t *)luaL_checkudata(L, 1, LM_SOCKET);

  if (sock->conn != NULL) {
    close(sock->conn->fd);
    mpf_recycle(sock->conn);
  }

  return 0;
}


static const struct luaL_Reg
as_lm_socket_functions[] = {
  {"new", lcf_socket_new},
  {NULL, NULL},
};


static const struct luaL_Reg
as_lm_socket_methods[] = {
  {"get_fd", lcf_socket_get_fd},
  {"close", lcf_socket_close},
  {"__gc", lcf_socket_destroy},
  {NULL, NULL},
};


int
luaopen_lm_socket(lua_State *L) {
  aluaL_newmetatable_with_methods(L, LM_SOCKET, as_lm_socket_methods);
  aluaL_newlib(L, "lm_socket", as_lm_socket_functions);

  return 1;
}
