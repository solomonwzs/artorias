#ifndef __LUA_OUTPUT__
#define __LUA_OUTPUT__

#include <lua.h>

extern int
loutput_redis_ok(lua_State *L, int fd);

#endif
