#ifndef __LUA_OUTPUT__
#define __LUA_OUTPUT__

#include <unistd.h>
#include "lua_bind.h"

extern int
loutput_redis_ok(lua_State *L, int fd);

#endif
