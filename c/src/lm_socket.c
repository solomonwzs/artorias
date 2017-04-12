#include "lm_socket.h"
#include "lua_utils.h"
#include "mem_pool.h"


// [-2,  +1, e]
static int
lcf_socket_new(lua_State *L) {
  as_lm_socket_t *sock;
  int ot = luaL_checkinteger(L, 1);

  lua_pushstring(L, LRK_MEM_POOL);
  lua_gettable(L, LUA_REGISTRYINDEX);
  as_mem_pool_fixed_t *mp = (as_mem_pool_fixed_t *)lua_touserdata(L, -1);

  sock = (as_lm_socket_t *)lua_newuserdata(L, sizeof(as_lm_socket_t));
  sock->ot_secs = ot;
  sock->conn = NULL;

  return 1;
}
