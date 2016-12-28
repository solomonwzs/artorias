#ifndef __LUA_PCONF__
#define __LUA_PCONF__

#include <lua.h>

typedef struct {
  lua_State *L;
} as_lua_pconf_t;

extern as_lua_pconf_t *
lpconf_new(const char *filename);

extern void
lpconf_destroy(as_lua_pconf_t *cnf);

#endif
