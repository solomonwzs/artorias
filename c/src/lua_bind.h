#ifndef __LUA_BIND__
#define __LUA_BIND__

#include <lua.h>
#include <lualib.h>
#include <lauxlib.h>
#include "utils.h"

#define AS_LB_OK  0x00

extern int
lbind_dofile(lua_State *L, const char *filename);

#endif
