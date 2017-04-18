#include "lua_utils.h"


static const struct luaL_Reg
as_lm_base_functions[] = {
  {NULL, NULL},
};


int
luaopen_lm_base(lua_State *L) {
  aluaL_newlib(L, "lm_base", as_lm_base_functions);

  lua_pushinteger(L, LAS_WAIT_FOR_INPUT);
  lua_setfield(L, -2, "WAIT_FOR_INPUT");

  lua_pushinteger(L, LAS_WAIT_FOR_OUTPUT);
  lua_setfield(L, -2, "WAIT_FOR_OUTPUT");

  lua_pushinteger(L, LAS_READY_TO_INPUT);
  lua_setfield(L, -2, "READY_TO_INPUT");

  lua_pushinteger(L, LAS_READY_TO_OUTPUT);
  lua_setfield(L, -2, "READY_TO_OUTPUT");

  lua_pushinteger(L, LAS_SOCKET_CLOSEED);
  lua_setfield(L, -2, "SOCKET_CLOSED");

  lua_pushinteger(L, EAGAIN);
  lua_setfield(L, -2, "EAGAIN");

  return 1;
}
