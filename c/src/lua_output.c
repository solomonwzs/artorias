#include <unistd.h>
#include <lualib.h>
#include <lauxlib.h>
#include <sys/socket.h>
#include "lua_utils.h"
#include "lua_output.h"


static int
lcf_write_redis_ok(lua_State *L) {
  int fd = lua_tointeger(L, -1);

  lua_getglobal(L, "redis_ok");
  int ret = lua_pcall(L, 0, 2, 0);
  if (ret != LUA_OK) {
    lb_pop_error_msg(L);
    send(fd, "-lua error\r\n", 12, MSG_NOSIGNAL);
    lua_pushinteger(L, ret);
    return 1;
  }

  int len = lua_tointeger(L, -1);
  const char *msg = lua_tostring(L, -2);
  send(fd, msg, len, MSG_NOSIGNAL);
  lua_pushinteger(L, LUA_OK);

  return 1;
}


int
loutput_redis_ok(lua_State *L, int fd) {
  lua_pushcfunction(L, lcf_write_redis_ok);
  lua_pushnumber(L, fd);
  int ret = lua_pcall(L, 1, 1, 0);

  if (ret == LUA_OK) {
    int r = lua_tointeger(L, -1);
    lua_pop(L, 1);
    return r;
  } else {
    lb_pop_error_msg(L);
    return ret;
  }
}
