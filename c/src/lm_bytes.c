#include "bytes.h"
#include "lm_bytes.h"
#include "lua_utils.h"
#include "lua_bind.h"
#include "mw_worker.h"

#define to_block(_n_) (container_of(_n_, as_bytes_block_t, node))


// [-1, +1, e]
int
lcf_bytes_read_from_fd(lua_State *L) {
  int fd = luaL_checkinteger(L, 1);

  lbind_checkmetatable(L, LRK_BYTES_REF_TABLE, "bytes ref table not exist");
  lbind_checkregvalue(L, LRK_WORKER_CTX, LUA_TLIGHTUSERDATA,
                      "no worker ctx");
  as_mw_worker_ctx_t *ctx = (as_mw_worker_ctx_t *)lua_touserdata(L, -1);

  as_bytes_t *bs = lua_newuserdata(L, sizeof(as_bytes_t));
  bytes_init(bs, ctx->mem_pool);

  size_t size = bytes_read_from_fd(bs, fd);
  if (size == -1) {
    lua_pushnil(L);
    return 1;
  }

  luaL_getmetatable(L, LM_BYTES);
  lua_setmetatable(L, -2);

  as_lm_bytes_ref_t *br = lua_newuserdata(L, sizeof(as_lm_bytes_ref_t));
  br->ori = bs;
  br->b_start = to_block(bs->dl.head);
  br->b_offset = 0;
  br->len = size;

  luaL_getmetatable(L, LM_BYTES_REF);
  lua_setmetatable(L, -2);

  lua_pushvalue(L, -1);
  lua_pushvalue(L, -2);
  lua_settable(L, -4);

  return 1;
}


// [-1, +1, e]
static int
lcf_bytes_ref_get_length(lua_State *L) {
  as_lm_bytes_ref_t *br = (as_lm_bytes_ref_t *)luaL_checkudata(
      L, 1, LM_BYTES);
  lua_pushinteger(L, br->len);
  return 1;
}


// [-1, +1, e]
static int
lcf_bytes_get_length(lua_State *L) {
  as_bytes_t *bs = (as_bytes_t *)luaL_checkudata(L, 1, LM_BYTES);
  lua_pushinteger(L, bs->used);
  return 1;
}


// [-1, +0, e]
static int
lcf_bytes_destroy(lua_State *L) {
  as_bytes_t *bs = (as_bytes_t *)luaL_checkudata(L, 1, LM_BYTES);
  bytes_destroy(bs);
  return 0;
}


static const struct luaL_Reg
as_lm_bytes_methods[] = {
  {"get_length", lcf_bytes_get_length},

  {"__gc", lcf_bytes_destroy},

  {NULL, NULL}
};


static const struct luaL_Reg
as_lm_bytes_ref_methods[] = {
  {"get_length", lcf_bytes_ref_get_length},

  {NULL, NULL}
};


void
inc_lm_bytes_metatable(lua_State *L) {
  aluaL_newmetatable_with_methods(L, LM_BYTES, as_lm_bytes_methods);
  aluaL_newmetatable_with_methods(L, LM_BYTES_REF, as_lm_bytes_ref_methods);
}
