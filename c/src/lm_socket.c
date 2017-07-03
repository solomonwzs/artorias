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
  as_mw_worker_fd_t *rfd = (as_mw_worker_fd_t *)sock->res->d;
  lua_pushfstring(L, "%d", rfd->fd);
  return 1;
}


// [-1, +3, e]
static int
lcf_socker_read(lua_State *L) {
  as_lm_socket_t *sock = (as_lm_socket_t *)luaL_checkudata(
      L, 1, LM_SOCKET);
  int n = luaL_checkinteger(L, 2);
  if (n <= 0) {
    n = 1024;
  }

  as_mw_worker_fd_t *rfd = (as_mw_worker_fd_t *)sock->res->d;
  char *buf = (char *)lua_newuserdata(L, n);

  int nbyte = read(rfd->fd, buf, n);
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
// 
// 
// // [-2, +0, e]
// static int
// lcf_sock_set_event(lua_State *L) {
//   as_lm_socket_t *sock = (as_lm_socket_t *)luaL_checkudata(
//       L, 1, LM_SOCKET);
//   as_wrap_conn_t *wc = sock->conn;
//   if (wc == NULL) {
//     lua_pushstring(L, "conn closed");
//     lua_error(L);
//   }
//   int event = luaL_checkinteger(L, 2);
// 
//   lua_pushstring(L, LRK_SERVER_EPFD);
//   lua_gettable(L, LUA_REGISTRYINDEX);
//   int epfd = lua_tointeger(L, -1);
// 
//   struct epoll_event e;
//   e.data.ptr = wc;
//   if (event == SOCK_EVENT_IN) {
//     e.events = EPOLLIN | EPOLLET;
//   } else if (event == SOCK_EVENT_OUT) {
//     e.events = EPOLLOUT | EPOLLET;
//   } else {
//     e.events = EPOLLET;
//   }
// 
//   as_sl_conn_t *sc = (as_sl_conn_t *)wc->d;
//   epoll_ctl(epfd, EPOLL_CTL_MOD, sc->fd, &e);
// 
//   return 0;
// }
// 
// 
// // [-1, +2, e]
// static int
// lcf_socket_send(lua_State *L) {
//   size_t len;
//   as_lm_socket_t *sock = (as_lm_socket_t *)luaL_checkudata(
//       L, 1, LM_SOCKET);
//   as_wrap_conn_t *wc = sock->conn;
//   if (wc == NULL) {
//     lua_pushstring(L, "conn closed");
//     lua_error(L);
//   }
// 
//   as_sl_conn_t *sc = (as_sl_conn_t *)wc->d;
//   const char *buf = lua_tolstring(L, 2, &len);
//   int nbyte = send(sc->fd, buf, len, MSG_NOSIGNAL);
//   if (nbyte < 0) {
//     lua_pushinteger(L, 0);
//     lua_pushinteger(L, errno);
//   } else {
//     lua_pushinteger(L, nbyte);
//     lua_pushnil(L);
//   }
// 
//   return 2;
// }
// 
// 
// // [-1, +3, e]
// static int
// lcf_socket_read(lua_State *L) {
//   as_lm_socket_t *sock = (as_lm_socket_t *)luaL_checkudata(
//       L, 1, LM_SOCKET);
//   as_wrap_conn_t *wc = sock->conn;
//   if (wc == NULL) {
//     lua_pushstring(L, "conn closed");
//     lua_error(L);
//   }
// 
//   int n = luaL_checkinteger(L, 2);
//   if (n == 0) {
//     n = 1024;
//   }
// 
//   char *buf = (char *)lua_newuserdata(L, n);
//   as_sl_conn_t *sc = (as_sl_conn_t *)wc->d;
//   int nbyte = read(sc->fd, buf, n);
//   if (nbyte < 0) {
//     lua_pushinteger(L, 0);
//     lua_pushnil(L);
//     lua_pushinteger(L, errno);
//   } else {
//     lua_pushinteger(L, nbyte);
//     lua_pushlstring(L, buf, nbyte);
//     lua_pushnil(L);
//   }
//   return 3;
// }
// 
// 
// // [-0, +1, e]
// static int
// lcf_socket_get_fd(lua_State *L) {
//   as_lm_socket_t *sock = (as_lm_socket_t *)luaL_checkudata(
//       L, 1, LM_SOCKET);
//   lua_pushinteger(L, ((as_sl_conn_t *)sock->conn->d)->fd);
// 
//   return 1;
// }


// // [-0, +0, e]
// static int
// lcf_socket_close(lua_State *L) {
//   as_lm_socket_t *sock = (as_lm_socket_t *)luaL_checkudata(
//       L, 1, LM_SOCKET);
// 
//   // lua_pushstring(L, LRK_WORKER_CTX);
//   // lua_gettable(L, LUA_REGISTRYINDEX);
//   // as_mw_worker_ctx_t *ctx = (as_mw_worker_ctx_t *)lua_touserdata(L, -1);
// 
//   if (sock->res == NULL || sock->res->resf == NULL || 
//       sock->type == LM_SOCK_TYPE_SYSTEM) {
//     return 0;
//   }
// 
//   as_mw_worker_fd_t *rfd = (as_mw_worker_fd_t *)sock->res->d;
// 
//   as_wrap_conn_t *wc = sock->conn;
//   if (wc != NULL) {
//     as_sl_conn_t *sc = (as_sl_conn_t *)wc->d;
//     close(sc->fd);
//     mpf_recycle(wc);
//     sock->conn = NULL;
//   }
// 
//   return 0;
// }


// // [-0, +0, e]
// static int
// lcf_socket_destroy(lua_State *L) {
//   as_lm_socket_t *sock = (as_lm_socket_t *)luaL_checkudata(L, 1, LM_SOCKET);
// 
//   as_wrap_conn_t *wc = sock->conn;
//   if (wc != NULL) {
//     as_sl_conn_t *sc = (as_sl_conn_t *)wc->d;
//     close(sc->fd);
//     mpf_recycle(wc);
//   }
// 
//   return 0;
// }
// 
// 
// static const struct luaL_Reg
// as_lm_socket_functions[] = {
//   {"new", lcf_socket_new},
//   {NULL, NULL},
// };
// 
// 
// static const struct luaL_Reg
// as_lm_socket_methods[] = {
//   {"send", lcf_socket_send},
//   {"read", lcf_socket_read},
//   {"set_event", lcf_sock_set_event},
//   {"get_fd", lcf_socket_get_fd},
//   {"close", lcf_socket_close},
//   {"__gc", lcf_socket_destroy},
//   {NULL, NULL},
// };
// 
// 
// int
// luaopen_lm_socket(lua_State *L) {
//   aluaL_newmetatable_with_methods(L, LM_SOCKET, as_lm_socket_methods);
//   aluaL_newlib(L, "lm_socket", as_lm_socket_functions);
// 
//   lua_pushinteger(L, SOCK_EVENT_IN);
//   lua_setfield(L, -2, "SOCK_EVENT_IN");
// 
//   lua_pushinteger(L, SOCK_EVENT_OUT);
//   lua_setfield(L, -2, "SOCK_EVENT_OUT");
// 
//   lua_pushinteger(L, SOCK_EVENT_NONE);
//   lua_setfield(L, -2, "SOCK_EVENT_NONE");
// 
//   return 1;
// }
