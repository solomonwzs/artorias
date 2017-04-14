#include "lm_socket.h"
#include "lua_utils.h"
#include "mem_pool.h"


// [-3,  +1, e]
static int
lcf_socket_new(lua_State *L) {
  as_lm_socket_t *sock;
  const char *host = luaL_checkstring(L, 1);
  uint16_t port = luaL_checkinteger(L, 2);
  int ot = luaL_checkinteger(L, 3);

  lua_pushstring(L, LRK_MEM_POOL);
  lua_gettable(L, LUA_REGISTRYINDEX);
  as_mem_pool_fixed_t *mp = (as_mem_pool_fixed_t *)lua_touserdata(L, -1);

  sock = (as_lm_socket_t *)lua_newuserdata(L, sizeof(as_lm_socket_t));
  sock->ot_secs = ot;
  sock->conn = NULL;

  return 1;
}
