#include <sys/socket.h>
#include "utils.h"
#include "lua_utils.h"
#include "lua_bind.h"
#include "mem_pool.h"
#include "lm_tsocket.h"


// [-0, +1, e]
static int
lcf_tsocket_new(lua_State *L) {
  lbind_checkmetatable(L, LRK_THREAD_LOCAL_VAR_TABLE,
                       "thread local var table not exist");
  lua_pushthread(L);
  lua_gettable(L, -2);
  lua_pushstring(L, "fd");
  lua_gettable(L, -2);
  int fd = lua_tointeger(L, -1);

  as_lm_tsocket_t *tsocket =  (as_lm_tsocket_t *)lua_newuserdata(
      L, sizeof(as_lm_tsocket_t));
  tsocket->fd = fd;

  luaL_getmetatable(L, LM_TSOCKET);
  lua_setmetatable(L, -2);

  return 1;
}


// [-0, +1, e]
static int
lcf_tsocket_tostring(lua_State *L) {
  as_lm_tsocket_t *tsocket = (as_lm_tsocket_t *)luaL_checkudata(
      L, 1, LM_TSOCKET);
  lua_pushfstring(L, "%d", tsocket->fd);
  return 1;
}


// [-1, +3, e]
static int
lcf_tsocket_read(lua_State *L) {
  as_lm_tsocket_t *tsocket = (as_lm_tsocket_t *)luaL_checkudata(
      L, 1, LM_TSOCKET);
  int n = luaL_checkinteger(L, 2);
  if (n == 0) {
    n = 1024;
  }

  char *buf = (char *)lua_newuserdata(L, n);
  int nbyte = read(tsocket->fd, buf, n);
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


// [-1, +2, e]
static int
lcf_tsocket_send(lua_State *L) {
  size_t len;
  as_lm_tsocket_t *tsocket = (as_lm_tsocket_t *)luaL_checkudata(
      L, 1, LM_TSOCKET);
  const char *buf = lua_tolstring(L, 2, &len);
  int nbyte = send(tsocket->fd, buf, len, MSG_NOSIGNAL);
  if (nbyte < 0) {
    lua_pushinteger(L, 0);
    lua_pushinteger(L, errno);
  } else {
    lua_pushinteger(L, nbyte);
    lua_pushnil(L);
  }

  return 2;
}


static const struct luaL_Reg
as_lm_tsocket_methods[] = {
  {"read", lcf_tsocket_read},
  {"send", lcf_tsocket_send},
  {"__tostring", lcf_tsocket_tostring},
  {NULL, NULL},
};


static const struct luaL_Reg
as_lm_tsocket_functions[] = {
  {"get", lcf_tsocket_new},
  {NULL, NULL},
};


int
luaopen_lm_tsocket(lua_State *L) {
  // luaL_newmetatable(L, LM_TSOCKET);
  // lua_pushvalue(L, -1);
  // lua_setfield(L, -2, "__index");
  // luaL_setfuncs(L, as_lm_tsocket_methods, 0);
  // luaL_newlib(L, as_lm_tsocket_functions);

  aluaL_newmetatable_with_methods(L, LM_TSOCKET, as_lm_tsocket_methods);
  aluaL_newlib(L, "lm_tsocket", as_lm_tsocket_functions);

  lua_pushinteger(L, LAS_WAIT_FOR_INPUT);
  lua_setfield(L, -2, "WAIT_FOR_INPUT");

  lua_pushinteger(L, LAS_WAIT_FOR_OUTPUT);
  lua_setfield(L, -2, "WAIT_FOR_OUTPUT");

  lua_pushinteger(L, LAS_READY_TO_INPUT);
  lua_setfield(L, -2, "READY_TO_INPUT");

  lua_pushinteger(L, LAS_READY_TO_OUTPUT);
  lua_setfield(L, -2, "READY_TO_OUTPUT");

  lua_pushinteger(L, EAGAIN);
  lua_setfield(L, -2, "EAGAIN");

  return 1;
}
