#include <sys/epoll.h>
#include <sys/socket.h>
#include "mw_worker.h"
#include "lm_socket.h"
#include "lua_utils.h"
#include "lua_bind.h"
#include "mem_pool.h"
#include "server.h"

#define SOCK_EVENT_IN   0x00
#define SOCK_EVENT_OUT  0x01
#define SOCK_EVENT_NONE 0x02


// [-0, +1, e]
static int
lcf_socket_get_msock(lua_State *L) {
  lbind_checkmetatable(L, LRK_THREAD_LOCAL_VAR_TABLE,
                       "thread local var table not exist");
  lua_pushthread(L);
  lua_gettable(L, -2);

  lua_pushstring(L, "th");
  lua_gettable(L, -2);
  as_thread_t *th = (as_thread_t *)lua_touserdata(L, -1);

  as_lm_socket_t *sock = (as_lm_socket_t *)lua_newuserdata(
      L, sizeof(as_lm_socket_t));
  sock->res = th->mfd_res;
  sock->type = LM_SOCK_TYPE_SYSTEM;

  luaL_getmetatable(L, LM_SOCKET);
  lua_setmetatable(L, -2);

  return 1;
}


// [-0, +1, e]
static int
lcf_socket_tostring(lua_State *L) {
  as_lm_socket_t *sock = (as_lm_socket_t *)luaL_checkudata(
      L, 1, LM_SOCKET);

  int fd = sock->res->fdf(sock->res);
  lua_pushfstring(L, "(addr: %p, fd: %d)", sock->res, fd);

  return 1;
}


// [-2, +3, e]
static int
lcf_socket_read(lua_State *L) {
  as_lm_socket_t *sock = (as_lm_socket_t *)luaL_checkudata(
      L, 1, LM_SOCKET);
  int n = luaL_checkinteger(L, 2);
  if (n <= 0) {
    n = 1024;
  }

  int fd = sock->res->fdf(sock->res);
  char *buf = (char *)lua_newuserdata(L, n);

  int nbyte = read(fd, buf, n);
  if (nbyte < 0) {
    lua_pushinteger(L, 0);
    lua_pushnil(L);
    lua_pushinteger(L, errno);
  } else {
    lua_pushinteger(L, nbyte);
    lua_pushlstring(L, buf, nbyte);
    lua_pushnil(L);
  }

  return 3;
}


// // [-3, +1, e]
// static int
// lcf_socket_new(lua_State *L) {
//   as_lm_socket_t *sock;
//   const char *host = luaL_checkstring(L, 1);
//   uint16_t port = luaL_checkinteger(L, 2);
//   int ot = luaL_checkinteger(L, 3);
// 
//   lua_pushstring(L, LRK_SERVER_EPFD);
//   lua_gettable(L, LUA_REGISTRYINDEX);
//   int epfd = lua_tointeger(L, -1);
// 
//   lua_pushstring(L, LRK_MEM_POOL);
//   lua_gettable(L, LUA_REGISTRYINDEX);
//   as_mem_pool_fixed_t *mp = (as_mem_pool_fixed_t *)lua_touserdata(L, -1);
// 
//   sock = (as_lm_socket_t *)lua_newuserdata(L, sizeof(as_lm_socket_t));
//   sock->ot_secs = ot;
//   sock->conn = NULL;
// 
//   luaL_getmetatable(L, LM_SOCKET);
//   lua_setmetatable(L, -2);
// 
//   sock->conn = (as_wrap_conn_t *)mpf_alloc(mp, sizeof_wrap_conn(as_sl_conn_t));
//   if (sock->conn == NULL) {
//     lua_pushstring(L, "alloc error");
//     lua_error(L);
//   }
//   sock->conn->type = AS_WC_SL_CONN;
// 
//   int fd = make_client_socket(host, port);
//   if (fd < 0) {
//     mpf_recycle(sock->conn);
//     lua_pushstring(L, "new socket error");
//     lua_error(L);
//   }
// 
//   as_sl_conn_t *sc = (as_sl_conn_t *)sock->conn->d;
//   sc->T = L;
//   sc->fd = fd;
// 
//   struct epoll_event e;
//   set_non_block(fd);
//   e.data.ptr = sock->conn;
//   e.events = EPOLLET;
//   if (epoll_ctl(epfd, EPOLL_CTL_ADD, fd, &e) != 0) {
//     mpf_recycle(sock->conn);
//     close(fd);
//     lua_pushstring(L, "epoll error");
//     lua_error(L);
//   }
// 
//   lbind_ref_fd_lthread(L, fd);
// 
//   return 1;
// }


// [-2, +2, e]
static int
lcf_socket_send(lua_State *L) {
  size_t len;
  as_lm_socket_t *sock = (as_lm_socket_t *)luaL_checkudata(L, 1, LM_SOCKET);

  as_thread_res_t *res = sock->res;
  if (res == NULL) {
    lua_pushstring(L, "conn closed");
    lua_error(L);
  }

  const char *buf = lua_tolstring(L, 2, &len);
  int fd = res->fdf(res);
  int nbyte = send(fd, buf, len, MSG_NOSIGNAL);
  if (nbyte < 0) {
    lua_pushinteger(L, 0);
    lua_pushinteger(L, errno);
  } else {
    lua_pushinteger(L, nbyte);
    lua_pushnil(L);
  }

  return 2;
}


// [-1, +1, e]
static int
lcf_socket_get_res(lua_State *L) {
  as_lm_socket_t *sock = (as_lm_socket_t *)luaL_checkudata(L, 1, LM_SOCKET);

  lua_pushlightuserdata(L, sock->res);

  return 1;
}


// [-1, +0, e]
static int
lcf_socket_close(lua_State *L) {
  as_lm_socket_t *sock = (as_lm_socket_t *)luaL_checkudata(L, 1, LM_SOCKET);

  if (sock->res == NULL || sock->type == LM_SOCK_TYPE_SYSTEM) {
    return 0;
  }
  asthread_res_del(sock->res->th, sock->res);

  if (sock->res->freef != NULL) {
    sock->res->freef(sock->res, NULL);
  }

  mpf_recycle(sock->res);
  sock->res = NULL;

  return 0;
}


static const struct luaL_Reg
as_lm_socket_functions[] = {
  {"get_msock", lcf_socket_get_msock},
  {NULL, NULL},
};


static const struct luaL_Reg
as_lm_socket_methods[] = {
  {"send", lcf_socket_send},
  {"read", lcf_socket_read},
  {"close", lcf_socket_close},
  {"get_res", lcf_socket_get_res},
  {"__tostring", lcf_socket_tostring},
  {"__gc", lcf_socket_close},
  {NULL, NULL},
};


int
luaopen_lm_socket(lua_State *L) {
  aluaL_newmetatable_with_methods(L, LM_SOCKET, as_lm_socket_methods);
  aluaL_newlib(L, "lm_socket", as_lm_socket_functions);

  lua_pushinteger(L, LAS_YIELD_FOR_IO);
  lua_setfield(L, -2, "YIELD_FOR_IO");

  lua_pushinteger(L, LAS_RESUME_IO);
  lua_setfield(L, -2, "RESUME_IO");

  lua_pushinteger(L, LAS_WAIT_FOR_INPUT);
  lua_setfield(L, -2, "WAIT_FOR_INPUT");

  lua_pushinteger(L, LAS_WAIT_FOR_OUTPUT);
  lua_setfield(L, -2, "WAIT_FOR_OUTPUT");

  lua_pushinteger(L, LAS_READY_TO_INPUT);
  lua_setfield(L, -2, "READY_TO_INPUT");

  lua_pushinteger(L, LAS_READY_TO_OUTPUT);
  lua_setfield(L, -2, "READY_TO_OUTPUT");

  return 1;
}
