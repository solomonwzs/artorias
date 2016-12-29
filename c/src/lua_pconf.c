#include <lauxlib.h>
#include "lua_pconf.h"
#include "lua_bind.h"
#include "utils.h"


static int
lcf_init_conf_L(lua_State *L) {
  lua_getglobal(L, "as_config");
  return 1;
}


as_lua_pconf_t *
lpconf_new(const char *filename) {
  int ret;
  as_lua_pconf_t *cnf = as_malloc(sizeof(as_lua_pconf_t));
  if (cnf == NULL) {
    return NULL;
  }

  lua_State *L = luaL_newstate();

  // cnf->L = luaL_newstate();
  // if ((ret = lbind_dofile(cnf->L, filename)) != LUA_OK) {
  //   lua_close(cnf->L);
  //   as_free(cnf);
  //   return NULL;
  // }

  // int t = lua_getglobal(cnf->L, "as_config");
  // debug_log("%d\n", t);
  // lua_pushstring(cnf->L, "tcp_port");
  // lua_gettable(cnf->L, -2);
  // int i = lua_tointeger(cnf->L, -1);
  // debug_log("%d\n", LUA_TTABLE);

  return cnf;
}


void
lpconf_destroy(as_lua_pconf_t *cnf) {
  if (cnf == NULL) {
    return;
  }
  lua_close(cnf->_L);
  as_free(cnf);
}
