#include "lua_bind.h"


static inline int
pop_pcall_rcode(lua_State *L, int pcall_ret) {
  if (pcall_ret != LUA_OK) {
    lb_error_msg(L);
    return pcall_ret;
  } else {
    int r = lua_tointeger(L, -1);
    lua_pop(L, 1);
    return r;
  }
}


static int
lcf_dofile(lua_State *L) {
  const char *filename = (const char *)lua_touserdata(L, -1);
  int ret = luaL_loadfile(L, filename);

  if (ret != LUA_OK) {
    lb_error_msg(L);
    lua_pushinteger(L, ret);
    return 1;
  }

  ret = lua_pcall(L, 0, 0, 0);
  if (ret != LUA_OK) {
    lb_error_msg(L);
  }
  lua_pushinteger(L, ret);
  return 1;
}


int
lbind_dofile(lua_State *L, const char *filename) {
  lua_pushcfunction(L, lcf_dofile);
  lua_pushlightuserdata(L, (void *)filename);
  int ret = lua_pcall(L, 1, 1, 0);
  return pop_pcall_rcode(L, ret);
}


static int
lcf_init_state(lua_State *L) {
  luaL_openlibs(L);
  lua_pushinteger(L, LUA_OK);
  // int ret = lbind_dofile(L, "src/lua/foo.lua");
  // lua_pushinteger(L, ret);
  return 1;
}


int
lbind_init_state(lua_State *L) {
  lua_pushcfunction(L, lcf_init_state);
  int ret = lua_pcall(L, 0, 1, 0);
  return pop_pcall_rcode(L, ret);
}
