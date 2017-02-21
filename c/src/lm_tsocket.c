#include <lua.h>
#include <lauxlib.h>
#include "utils.h"
#include "lua_utils.h"
#include "mem_pool.h"
#include "lm_tsocket.h"


// [-0, +1, e]
static int
lcf_tsocket_new(lua_State *L) {
  int ok = luaL_getmetatable(L, LRK_THREAD_LOCAL_VAR_TABLE);
  if (!ok) {
    lua_pushstring(L, "thread local var table not exist");
    lua_error(L);
  }
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


static const struct luaL_Reg
as_lm_tsocket_methods[] = {
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
  luaL_newmetatable(L, LM_TSOCKET);
  lua_pushvalue(L, -1);
  lua_setfield(L, -2, "__index");
  luaL_setfuncs(L, as_lm_tsocket_methods, 0);
  luaL_newlib(L, as_lm_tsocket_functions);

  return 1;
}
