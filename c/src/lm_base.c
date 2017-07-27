#include <sys/epoll.h>
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

  // S
  lua_newtable(L);

  lua_pushinteger(L, LAS_S_YIELD_FOR_IO);
  lua_setfield(L, -2, "YIELD_FOR_IO");

  lua_pushinteger(L, LAS_S_RESUME_IO);
  lua_setfield(L, -2, "RESUME_IO");

  lua_pushinteger(L, LAS_S_WAIT_FOR_INPUT);
  lua_setfield(L, -2, "WAIT_FOR_INPUT");

  lua_pushinteger(L, LAS_S_WAIT_FOR_OUTPUT);
  lua_setfield(L, -2, "WAIT_FOR_OUTPUT");

  lua_pushinteger(L, LAS_S_READY_TO_INPUT);
  lua_setfield(L, -2, "READY_TO_INPUT");

  lua_pushinteger(L, LAS_S_READY_TO_OUTPUT);
  lua_setfield(L, -2, "READY_TO_OUTPUT");

  lua_setfield(L, -2, "S");

  // E
  lua_newtable(L);

  lua_pushinteger(L, EAGAIN);
  lua_setfield(L, -2, "EAGAIN");

  lua_pushinteger(L, EBADFD);
  lua_setfield(L, -2, "EBADFD");

  lua_pushinteger(L, EINVAL);
  lua_setfield(L, -2, "EINVAL");

  lua_pushinteger(L, ETIMEDOUT);
  lua_setfield(L, -2, "ETIMEDOUT");

  lua_setfield(L, -2, "E");

  // D
  lua_newtable(L);

  lua_pushinteger(L, LAS_D_TIMOUT_SECS);
  lua_setfield(L, -2, "TIMEOUT_SECS");

  lua_setfield(L, -2, "D");

  // EV
  lua_newtable(L);

  lua_pushinteger(L, EPOLLIN);
  lua_setfield(L, -2, "EPOLLIN");

  lua_pushinteger(L, EPOLLOUT);
  lua_setfield(L, -2, "EPOLLOUT");

  lua_pushinteger(L, EPOLLET);
  lua_setfield(L, -2, "EPOLLET");

  lua_setfield(L, -2, "EV");

  return 1;
}
