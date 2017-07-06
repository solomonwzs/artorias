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


static int
res_free_f(as_thread_res_t *res, void *f_ptr) {
  int *fd = (int *)res->d;
  close(*fd);
  return 0;
}


static int
res_fd_f(as_thread_res_t *res) {
  return *(int *)res->d;
}


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


// [-2, +1, e]
static int
lcf_socket_new(lua_State *L) {
  const char *host = luaL_checkstring(L, 1);
  int port = luaL_checkinteger(L, 2);

  lbind_checkregvalue(L, LRK_WORKER_CTX, LUA_TLIGHTUSERDATA,
                      "no worker ctx");
  as_mw_worker_ctx_t *ctx = (as_mw_worker_ctx_t *)lua_touserdata(L, -1);

  lbind_checkmetatable(L, LRK_THREAD_LOCAL_VAR_TABLE,
                       "thread local var table not exist");
  lua_pushthread(L);
  lua_gettable(L, -2);

  lua_pushstring(L, "th");
  lua_gettable(L, -2);
  as_thread_t *th = (as_thread_t *)lua_touserdata(L, -1);

  as_lm_socket_t *sock = (as_lm_socket_t *)lua_newuserdata(
      L, sizeof(as_lm_socket_t));
  sock->res = NULL;
  sock->type = LM_SOCK_TYPE_CUSTOM;

  luaL_getmetatable(L, LM_SOCKET);
  lua_setmetatable(L, -2);

  sock->res = (as_thread_res_t *)mpf_alloc(
      ctx->mem_pool, sizeof_thread_res(int));
  if (sock->res == NULL) {
    lua_pushstring(L, "alloc error");
    lua_error(L);
  }

  int fd = make_client_socket(host, port);
  if (fd < 0) {
    mpf_recycle(sock->res);
    lua_pushstring(L, "connect error");
    lua_error(L);
  }
  set_non_block(fd);
  *((int *)sock->res->d) = fd;
  asthread_res_init(sock->res, res_free_f, res_fd_f);
  asthread_res_add_to_th(sock->res, th);

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
  asthread_res_del_from_th(sock->res, sock->res->th);

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
  {"new", lcf_socket_new},
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

  return 1;
}
