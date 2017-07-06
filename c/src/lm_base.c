#include "lua_utils.h"
#include "lua_bind.h"
#include "thread.h"


// [0, +1, e]
static int
lcf_get_tid(lua_State *L) {
  lbind_checkmetatable(L, LRK_THREAD_LOCAL_VAR_TABLE,
                       "thread local var table not exist");

  lua_pushthread(L);
  lua_gettable(L, -2);

  lua_pushstring(L, "th");
  lua_gettable(L, -2);
  as_thread_t *th = (as_thread_t *)lua_touserdata(L, -1);

  lua_pushinteger(L, th->tid);

  return 1;
}


static const struct luaL_Reg
as_lm_base_functions[] = {
  {"get_tid", lcf_get_tid},
  {NULL, NULL},
};


int
luaopen_lm_base(lua_State *L) {
  aluaL_newlib(L, "lm_base", as_lm_base_functions);

  lua_pushinteger(L, LAS_YIELD_FOR_IO);
  lua_setfield(L, -2, "YIELD_FOR_IO");

  lua_pushinteger(L, LAS_RESUME_IO);
  lua_setfield(L, -2, "RESUME_IO");

  lua_pushinteger(L, LAS_WAIT_FOR_INPUT);
  lua_setfield(L, -2, "WAIT_FOR_INPUT");

  lua_pushinteger(L, LAS_WAIT_FOR_OUTPUT);
  lua_setfield(L, -2, "WAIT_FOR_OUTPUT");

  lua_pushinteger(L, LAS_READY_TO_INPUT);
  lua_setfield(L, -2, "READY_TO_INPUT");

  lua_pushinteger(L, LAS_READY_TO_OUTPUT);
  lua_setfield(L, -2, "READY_TO_OUTPUT");

  lua_pushinteger(L, EAGAIN);
  lua_setfield(L, -2, "EAGAIN");

  return 1;
}
