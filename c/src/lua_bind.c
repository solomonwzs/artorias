#include <lualib.h>
#include <lauxlib.h>
#include <string.h>
#include "lua_bind.h"
#include "lua_utils.h"
#include "utils.h"


static int
lcf_dofile(lua_State *L) {
  const char *filename = (const char *)lua_touserdata(L, -1);
  int ret = luaL_loadfile(L, filename);

  if (ret != LUA_OK) {
    lb_error_msg(L);
    lua_pushinteger(L, ret);
    return 1;
  }

  ret = lua_pcall(L, 0, 0, 0);
  if (ret != LUA_OK) {
    lb_error_msg(L);
  }
  lua_pushinteger(L, ret);
  return 1;
}


int
lbind_dofile(lua_State *L, const char *filename) {
  lua_pushcfunction(L, lcf_dofile);
  lua_pushlightuserdata(L, (void *)filename);
  int ret = lua_pcall(L, 1, 1, 0);
  return pop_pcall_rcode(L, ret);
}


static int
lcf_init_state(lua_State *L) {
  luaL_openlibs(L);
  // lua_pushinteger(L, LUA_OK);
  int ret = lbind_dofile(L, "src/lua/foo.lua");
  lua_pushinteger(L, ret);
  return 1;
}


int
lbind_init_state(lua_State *L) {
  lua_pushcfunction(L, lcf_init_state);
  int ret = lua_pcall(L, 0, 1, 0);
  return pop_pcall_rcode(L, ret);
}


static void *
lalloc(void *ud, void *ptr, size_t osize, size_t nsize) {
  if (nsize == 0) {
    mpf_recycle(ptr);
    return NULL;
  } else {
    as_mem_pool_fixed_t *mp = (as_mem_pool_fixed_t *)ud;
    if (ptr != NULL) {
      if (osize >= nsize ||
          mpf_data_size(ptr) >= nsize) {
        return ptr;
      } else {
        void *nptr = mpf_alloc(mp, nsize);
        memcpy(nptr, ptr, osize);
        mpf_recycle(ptr);
        return nptr;
      }
    } else {
      size_t size;
      if (osize == LUA_TTABLE) {
        size = nsize * 4;
      } else {
        size = nsize;
      }
      return mpf_alloc(mp, size);
    }
  }
  return NULL;
}


lua_State *
lbind_new_state(as_mem_pool_fixed_t *mp) {
  if (mp == NULL) {
    return luaL_newstate();
  } else {
    return lua_newstate(lalloc, mp);
  }
}
