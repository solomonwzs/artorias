#include <lauxlib.h>
#include <stdarg.h>

#include "lua_pconf.h"
#include "lua_bind.h"
#include "lua_utils.h"
#include "utils.h"


static int
lcf_get_value(lua_State *L) {
  int n = lua_gettop(L);
  int i;
  lua_getglobal(L, "as_config");
  for (i = 1; i <= n; ++i) {
    if (!lua_istable(L, -1)) {
      lua_pushnil(L);
      return 1;
    }
    lua_pushvalue(L, i);
    lua_gettable(L, -2);
  }
  return 1;
}


as_lua_pconf_t *
lpconf_new(const char *filename) {
  as_lua_pconf_t *cnf = as_malloc(sizeof(as_lua_pconf_t));
  if (cnf == NULL) {
    return NULL;
  }

  lua_State *L = luaL_newstate();
  if (lbind_dofile(L, filename) != LUA_OK) {
    lua_close(L);
    as_free(L);
    return NULL;
  }

  cnf->_L = L;
  return cnf;
}


as_cnf_return_t
lpconf_get_integer_value(as_lua_pconf_t *cnf, int n, ...) {
  va_list ap;
  char *field;

  lua_pushcfunction(cnf->_L, lcf_get_value);
  va_start(ap, n);
  for (int i = 0; i < n; ++i) {
    field = va_arg(ap, char *);
    lua_pushstring(cnf->_L, field);
  }
  va_end(ap);

  as_cnf_return_t ret;
  int r = lua_pcall(cnf->_L, n, 1, 0);
  if (r != LUA_OK) {
    lb_error_msg(cnf->_L);
    ret.type = LUA_TNONE;
  } else {
    switch (lua_type(cnf->_L, -1)) {
      case LUA_TNIL:
        ret.type = LPCNF_TNIL;
        ret.val.p = NULL;
        break;
      case LUA_TNUMBER:
        ret.type = LPCNF_TNUMBER;
        ret.val.i = lua_tointeger(cnf->_L, -1);
        break;
      case LUA_TSTRING:
        ret.type = LPCNF_TNUMBER;
        ret.val.s = lua_tostring(cnf->_L, -1);
        break;
      default:
        ret.type = LUA_TNONE;
        break;
    }
    lua_pop(cnf->_L, -1);
  }
  return ret;
}


void
lpconf_destroy(as_lua_pconf_t *cnf) {
  if (cnf == NULL) {
    return;
  }
  lua_close(cnf->_L);
  as_free(cnf);
}
