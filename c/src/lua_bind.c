#include "lua_bind.h"

#define error_msg(_L_) do {\
  debug_log("lua_error: %s\n", lua_tostring(_L_, -1));\
  lua_pop(_L_, 1);\
} while (0)


static int
lcf_dofile(lua_State *L) {
  const char *filename = (const char *)lua_touserdata(L, -1);
  int ret = luaL_loadfile(L, filename);

  if (ret != LUA_OK) {
    error_msg(L);
    lua_pushinteger(L, ret);
    return 1;
  }

  ret = lua_pcall(L, 0, 0, 0);
  if (ret != LUA_OK) {
    error_msg(L);
  }
  lua_pushinteger(L, ret);
  return 1;
}


int
lbind_dofile(lua_State *L, const char *filename) {
  lua_pushcfunction(L, lcf_dofile);
  lua_pushlightuserdata(L, (void *)filename);
  int ret = lua_pcall(L, 1, 1, 0);
  if (ret != LUA_OK) {
    error_msg(L);
  } else {
    ret = lua_tointeger(L, -1);
    lua_pop(L, 1);
  }
  return ret;
}
