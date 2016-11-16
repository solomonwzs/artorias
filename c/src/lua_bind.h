#ifndef __LUA_BIND__
#define __LUA_BIND__

#include <lua.h>
#include "mem_pool.h"

extern int
lbind_dofile(lua_State *L, const char *filename);

extern int
lbind_init_state(lua_State *L);

extern lua_State *
lbind_new_state(as_mem_pool_fixed_t *mp);

#endif
