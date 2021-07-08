#ifndef __LUA_PCONF__
#define __LUA_PCONF__

#include "lua_utils.h"

#define LPCNF_TNONE   LUA_TNONE
#define LPCNF_TNIL    LUA_TNIL
#define LPCNF_TNUMBER LUA_TNUMBER
#define LPCNF_TSTRING LUA_TSTRING

union as_cnf_value_u {
  int i;
  const char *s;
  void *p;
};

typedef struct {
  int type;
  union as_cnf_value_u val;
} as_cnf_return_t;

typedef struct {
  lua_State *_L;
} as_lua_pconf_t;

extern as_lua_pconf_t *lpconf_new(const char *filename);

extern void lpconf_destroy(as_lua_pconf_t *cnf);

extern as_cnf_return_t lpconf_get_pconf_value(as_lua_pconf_t *cnf, int n, ...);

#endif
