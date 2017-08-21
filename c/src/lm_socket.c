#include <sys/epoll.h>
#include <sys/socket.h>
#include "bytes.h"
#include "lm_socket.h"
#include "lua_utils.h"
#include "lua_bind.h"
#include "mem_pool.h"
#include "mw_worker.h"
#include "server.h"

#define _SOCK_EVENT_IN   0x00
#define _SOCK_EVENT_OUT  0x01
#define _SOCK_EVENT_NONE 0x02

typedef struct {
  as_thread_res_t *res;
  const char      *buf;
  size_t          len;
  size_t          idx;
} _send_ctx_t;


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
  lbind_get_thread_local_vars(L, 1, LLK_THREAD);
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

  lbind_get_thread_local_vars(L, 1, LLK_THREAD);
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
  asthread_res_ev_init(sock->res, ctx->epfd);

  return 1;
}


// [-0, +1, e]
static int
lcf_socket_tostring(lua_State *L) {
  as_lm_socket_t *sock = (as_lm_socket_t *)luaL_checkudata(
      L, 1, LM_SOCKET);

  int fd = sock->res != NULL ? sock->res->fdf(sock->res) : -1;
  lua_pushfstring(L, "(s: %p, addr: %p, fd: %d)", sock, sock->res, fd);

  return 1;
}


// [-2, +3, e]
static int
lcf_socket_sread(lua_State *L) {
  as_lm_socket_t *sock = (as_lm_socket_t *)luaL_checkudata(
      L, 1, LM_SOCKET);
  int n = luaL_checkinteger(L, 2);
  if (n <= 0) {
    n = 1024;
  }

  as_thread_res_t *res = sock->res;
  if (res == NULL) {
    lua_pushinteger(L, 0);
    lua_pushnil(L);
    lua_pushinteger(L, EBADFD);
    return 3;
  }

  int fd = res->fdf(res);
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


// [-1, +3, e]
static int
lcf_socket_sread_all(lua_State *L) {
  as_lm_socket_t *sock = (as_lm_socket_t *)luaL_checkudata(
      L, 1, LM_SOCKET);
  as_thread_res_t *res = sock->res;
  if (res == NULL) {
    lua_pushinteger(L, 0);
    lua_pushnil(L);
    lua_pushinteger(L, EBADFD);
    return 3;
  }

  lbind_checkregvalue(L, LRK_WORKER_CTX, LUA_TLIGHTUSERDATA,
                      "no worker ctx");
  as_mw_worker_ctx_t *ctx = (as_mw_worker_ctx_t *)lua_touserdata(L, -1);

  int fd = res->fdf(res);
  as_bytes_t buf;
  bytes_init(&buf, ctx->mem_pool);
  size_t size = bytes_read_from_fd(&buf, fd);

  if (size == -1) {
    lua_pushinteger(L, 0);
    lua_pushnil(L);
    lua_pushinteger(L, errno);
  } else {
    void *ptr = lua_newuserdata(L, size);
    bytes_copy_to(&buf, ptr, 0, size);

    lua_pushinteger(L, size);
    lua_pushlstring(L, ptr, size);
    lua_pushnil(L);
  }

  bytes_destroy(&buf);
  return 3;
}


// [-2, +2, e]
static int
lcf_socket_ssend(lua_State *L) {
  as_lm_socket_t *sock = (as_lm_socket_t *)luaL_checkudata(L, 1, LM_SOCKET);

  as_thread_res_t *res = sock->res;
  if (res == NULL) {
    lua_pushinteger(L, 0);
    lua_pushinteger(L, EBADFD);
    return 2;
  }

  size_t len;
  const char *buf = lua_tolstring(L, 2, &len);
  if (buf == NULL) {
    lua_pushinteger(L, 0);
    lua_pushinteger(L, EINVAL);
    return 2;
  }

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


static int
k_socket_read(lua_State *L, int status, lua_KContext c) {
  as_thread_res_t *res = NULL;
  if (status == LUA_YIELD) {
    int type = luaL_checkinteger(L, 4);
    if (type == LAS_S_RESUME_IO_TIMEOUT) {
      lua_pushinteger(L, 0);
      lua_pushnil(L);
      lua_pushinteger(L, ETIMEDOUT);
      return 3;
    } else {
      res = (as_thread_res_t *)lua_touserdata(L, -2);
      lua_pop(L, 3);
    }
  } else {
    lua_pushstring(L, "read error");
    lua_error(L);
  }

  int n = luaL_checkinteger(L, 2);
  if (n <= 0) {
    n = 1024;
  }

  int fd = res->fdf(res);
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


// [-3, +3, e]
static int
lcf_socket_read(lua_State *L) {
  as_lm_socket_t *sock = (as_lm_socket_t *)luaL_checkudata(
      L, 1, LM_SOCKET);
  int timeout = luaL_checkinteger(L, 3);
  if (timeout < 0) {
    timeout = LAS_D_TIMOUT_SECS;
  }

  as_thread_res_t *res = sock->res;
  if (res == NULL) {
    lua_pushinteger(L, 0);
    lua_pushnil(L);
    lua_pushinteger(L, EBADFD);
    return 3;
  }

  if (res->th->mode != AS_TMODE_SIMPLE) {
    lua_pushstring(L, "ev error");
    lua_error(L);
  }

  lua_pushinteger(L, LAS_S_YIELD_FOR_IO);
  lua_pushlightuserdata(L, res);
  lua_pushinteger(L, LAS_S_WAIT_FOR_INPUT);
  lua_pushinteger(L, timeout);
  return lua_yieldk(L, 4, (lua_KContext)NULL, k_socket_read);
}


static int
k_socket_send(lua_State *L, int status, lua_KContext c) {
  as_thread_res_t *res = NULL;
  _send_ctx_t *ctx = (_send_ctx_t *)c;

  if (status == LUA_OK) {
    res = ctx->res;
  } else if (status == LUA_YIELD) {
    int type = luaL_checkinteger(L, 1);
    // as_thread_res_t *rres = (as_thread_res_t *)lua_touserdata(L, 2);
    // int io = luaL_checkinteger(L, 3);

    if (type == LAS_S_RESUME_IO_TIMEOUT) {
      lua_pushinteger(L, ctx->idx);
      lua_pushinteger(L, ETIMEDOUT);
      mpf_recycle(ctx);
      return 2;
    } else {
      res = (as_thread_res_t *)lua_touserdata(L, -2);
      lua_pop(L, 3);
    }
  } else {
    mpf_recycle(ctx);
    lua_pushstring(L, "send error");
    lua_error(L);
  }

  int fd = res->fdf(res);
  while (1) {
    const char *buf = ctx->buf + ctx->idx;
    size_t len = ctx->len - ctx->idx;
    ssize_t nbyte = send(fd, buf, len, MSG_NOSIGNAL);
    if (nbyte == len) {
      lua_pushinteger(L, ctx->len);
      lua_pushnil(L);
      break;
    } else if (nbyte == -1 && errno != EAGAIN) {
      lua_pushinteger(L, ctx->idx);
      lua_pushinteger(L, errno);
      break;
    } else if (nbyte == -1) {
      lua_pushinteger(L, LAS_S_YIELD_FOR_IO);
      lua_pushlightuserdata(L, res);
      lua_pushinteger(L, LAS_S_WAIT_FOR_OUTPUT);
      lua_pushinteger(L, LAS_D_TIMOUT_SECS);
      return lua_yieldk(L, 4, (lua_KContext)ctx, k_socket_send);
    } else {
      ctx->idx += nbyte;
    }
  }

  mpf_recycle(ctx);
  return 2;
}


// [-2, +2, e]
static int
lcf_socket_send(lua_State *L) {
  as_lm_socket_t *sock = (as_lm_socket_t *)luaL_checkudata(L, 1, LM_SOCKET);

  as_thread_res_t *res = sock->res;
  if (res == NULL) {
    lua_pushinteger(L, 0);
    lua_pushinteger(L, EBADFD);
    return 2;
  }

  if (res->th->mode != AS_TMODE_SIMPLE) {
    lua_pushstring(L, "ev error");
    lua_error(L);
  }

  size_t len;
  const char *buf = lua_tolstring(L, 2, &len);
  if (buf == NULL) {
    lua_pushinteger(L, 0);
    lua_pushinteger(L, EINVAL);
    return 2;
  }

  lbind_checkregvalue(L, LRK_WORKER_CTX, LUA_TLIGHTUSERDATA,
                      "no worker ctx");
  as_mw_worker_ctx_t *ctx = (as_mw_worker_ctx_t *)lua_touserdata(L, -1);

  _send_ctx_t *c = mpf_alloc(ctx->mem_pool, sizeof(_send_ctx_t));
  c->res = res;
  c->buf = buf;
  c->len = len;
  c->idx = 0;

  return k_socket_send(L, LUA_OK, (lua_KContext)c);
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

  if (sock->res->th != NULL) {
    asthread_res_del_from_th(sock->res, sock->res->th);
  }

  if (sock->res->freef != NULL) {
    sock->res->freef(sock->res, NULL);
  }

  mpf_recycle(sock->res);
  sock->res = NULL;

  return 0;
}


// [-?, +1, e]
static int
lcf_socket_ev_begin(lua_State *L) {
  lbind_checkregvalue(L, LRK_WORKER_CTX, LUA_TLIGHTUSERDATA,
                      "no worker ctx");
  as_mw_worker_ctx_t *ctx = (as_mw_worker_ctx_t *)lua_touserdata(L, -1);

  lbind_get_thread_local_vars(L, 1, LLK_THREAD);
  as_thread_t *th = (as_thread_t *)lua_touserdata(L, -1);

  lua_newtable(L);
  lbind_set_thread_local_vars(L, 1, LLK_RES_SOCK_TABLE, LTYPE_STACK, -1);

  int n = luaL_checkinteger(L, 1);
  int m = 0;
  for (int i = 1; i < n + 1; ++i) {
    as_lm_socket_t *sock = (as_lm_socket_t *)luaL_checkudata(
        L, i * 2, LM_SOCKET);
    int ev = luaL_checkinteger(L, i * 2 + 1);
    if (sock->res == NULL || sock->res->th != th) {
      break;
    }

    lua_pushlightuserdata(L, sock->res);
    lua_pushvalue(L, i * 2);
    lua_settable(L, -3);

    sock->res->status = AS_RSTATUS_EV;
    asthread_res_ev_add(sock->res, ctx->epfd, ev);
    m += 1;
  }
  th->mode = AS_TMODE_LOOP_SOCKS;

  lua_pushinteger(L, m);

  return 1;
}


// [-0, +0, e]
static int
lcf_socket_ev_end(lua_State *L) {
  lbind_get_thread_local_vars(L, 1, LLK_THREAD);
  as_thread_t *th = (as_thread_t *)lua_touserdata(L, -1);

  if (th == NULL || th->mode != AS_TMODE_LOOP_SOCKS) {
    return 0;
  }

  lbind_checkregvalue(L, LRK_WORKER_CTX, LUA_TLIGHTUSERDATA,
                      "no worker ctx");
  as_mw_worker_ctx_t *ctx = (as_mw_worker_ctx_t *)lua_touserdata(L, -1);

  th->mode = AS_TMODE_SIMPLE;
  asthread_remove_res_from_epfd(th, ctx->epfd);

  return 0;
}


static int
k_socket_ev_wait(lua_State *L, int status, lua_KContext c) {
  if (status == LUA_YIELD) {
    int type = luaL_checkinteger(L, 2);
    if (type == LAS_S_RESUME_IO) {

      // as_thread_res_t *res = (as_thread_res_t *)lua_touserdata(L, 3);
      // int rw = lua_tointeger(L, 4);

      lbind_get_thread_local_vars(L, 1, LLK_RES_SOCK_TABLE);

      lua_pushvalue(L, 2);
      lua_pushvalue(L, 3);
      lua_gettable(L, -3);
      lua_pushvalue(L, 4);

    } else if (type == LAS_S_RESUME_IO_ERROR){

      // as_thread_res_t *res = (as_thread_res_t *)lua_touserdata(L, 3);
      // int err = lua_tointeger(L, 4);

      lbind_get_thread_local_vars(L, 1, LLK_RES_SOCK_TABLE);

      lua_pushvalue(L, 2);
      lua_pushvalue(L, 3);
      lua_gettable(L, -3);
      lua_pushnil(L);

    } else if (type == LAS_S_RESUME_SLEEP) {

      lua_pushnil(L);
      lua_pushnil(L);

    }
  } else {
    lua_pushstring(L, "ev error");
    lua_error(L);
  }

  return 3;
}


// [-0, +3, e]
static int
lcf_socket_ev_wait(lua_State *L) {
  int timeout = luaL_checkinteger(L, 1);

  lua_pushinteger(L, LAS_S_YIELD_FOR_EV);
  lua_pushinteger(L, timeout);
  return lua_yieldk(L, 2, (lua_KContext)NULL, k_socket_ev_wait);
}


static const struct luaL_Reg
as_lm_socket_functions[] = {
  {"get_msock", lcf_socket_get_msock},
  {"new", lcf_socket_new},

  {"ev_begin", lcf_socket_ev_begin},
  {"ev_end", lcf_socket_ev_end},
  {"ev_wait", lcf_socket_ev_wait},

  {NULL, NULL},
};


static const struct luaL_Reg
as_lm_socket_methods[] = {
  {"ssend", lcf_socket_ssend},
  {"sread", lcf_socket_sread},

  {"sread_all", lcf_socket_sread_all},

  {"send", lcf_socket_send},
  {"read", lcf_socket_read},

  {"close", lcf_socket_close},
  {"get_res", lcf_socket_get_res},
  {"__tostring", lcf_socket_tostring},
  {"__gc", lcf_socket_close},
  {NULL, NULL},
};


void
inc_lm_socket_metatable(lua_State *L) {
  aluaL_newmetatable_with_methods(L, LM_SOCKET, as_lm_socket_methods);
}


int
luaopen_lm_socket(lua_State *L) {
  inc_lm_socket_metatable(L);
  aluaL_newlib(L, "lm_socket", as_lm_socket_functions);

  return 1;
}
