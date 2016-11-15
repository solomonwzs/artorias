#include "lua_output.h"


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
lcf_write_redis_ok(lua_State *L) {
  int fd = lua_tointeger(L, -1);

  lua_getglobal(L, "redis_ok");
  int ret = lua_pcall(L, 0, 2, 0);
  if (ret != LUA_OK) {
    lb_error_msg(L);
    write(fd, "-lua error\r\n", 12);
    lua_pushinteger(L, ret);
    return 1;
  }

  int len = lua_tointeger(L, -1);
  const char *msg = lua_tostring(L, -2);
  write(fd, msg, len);
  lua_pushinteger(L, LUA_OK);

  return 1;
}


int
loutput_redis_ok(lua_State *L, int fd) {
  lua_pushcfunction(L, lcf_write_redis_ok);
  lua_pushnumber(L, fd);
  int ret = lua_pcall(L, 1, 1, 0);
  return pop_pcall_rcode(L, ret);
}
