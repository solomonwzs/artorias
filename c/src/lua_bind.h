#ifndef __LUA_BIND__
#define __LUA_BIND__

#include <lua.h>
#include <lualib.h>
#include <lauxlib.h>
#include "utils.h"

#define lb_error_msg(_L_) do {\
  debug_log("lua_error: %s\n", lua_tostring(_L_, -1));\
  lua_pop(_L_, 1);\
} while (0)

extern int
lbind_dofile(lua_State *L, const char *filename);

extern int
lbind_init_state(lua_State *L);

#endif
