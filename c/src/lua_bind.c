#include <lualib.h>
#include <lauxlib.h>
#include <string.h>
#include "lua_bind.h"
#include "lua_utils.h"
#include "utils.h"


#define lbind_checkmetatable(_L_, _idx_, _emsg_) \
    if (!luaL_getmetatable(_L_, _idx_)) { \
      lua_pushstring(_L_, _emsg_);\
      lua_error(_L_);\
    }


// [-1, +0, e]
static int
lcf_dofile(lua_State *L) {
  const char *filename = (const char *)lua_touserdata(L, -1);

  int ret = luaL_loadfile(L, filename);
  if (ret != LUA_OK) {
    lua_error(L);
  }
  ret = lua_pcall(L, 0, 0, 0);
  if (ret != LUA_OK) {
    lua_error(L);
  }

  return 0;
}


// [-0, +0, -]
int
lbind_dofile(lua_State *L, const char *filename) {
  lua_pushcfunction(L, lcf_dofile);
  lua_pushlightuserdata(L, (void *)filename);

  int ret = lua_pcall(L, 1, 0, 0);
  if (ret != LUA_OK) {
    lb_pop_error_msg(L);
  }
  return ret;
}


// [-1, +0, e]
static int
lcf_init_state(lua_State *L) {
  lua_pushstring(L, LRK_MEM_POOL);
  lua_pushvalue(L, 1);
  lua_settable(L, LUA_REGISTRYINDEX);

  luaL_openlibs(L);

  luaL_newmetatable(L, LRK_THREAD_TABLE);

  luaL_newmetatable(L, LRK_LCODE_CHUNK_TABLE);

  luaL_newmetatable(L, LRK_THREAD_LOCAL_VAR_TABLE);
  lua_pushvalue(L, -1);
  lua_pushstring(L, "_mode");
  lua_pushstring(L, "k");
  lua_settable(L, -3);
  lua_setmetatable(L, -2);

  return 0;
}


// [-0, +0, -]
int
lbind_init_state(lua_State *L, as_mem_pool_fixed_t *mp) {
  lua_pushcfunction(L, lcf_init_state);
  lua_pushlightuserdata(L, (void *)mp);

  int ret = lua_pcall(L, 1, 0, 0);
  if (ret != LUA_OK) {
    lb_pop_error_msg(L);
  }
  return ret;
}


static void *
lalloc(void *ud, void *ptr, size_t osize, size_t nsize) {
  if (nsize == 0) {
    mpf_recycle(ptr);
    return NULL;
  } else {
    as_mem_pool_fixed_t *mp = (as_mem_pool_fixed_t *)ud;
    if (ptr != NULL) {
      return mpf_realloc(ptr, nsize);
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


// [-2, +0, e]
static int
lcf_append_lua_package_field(lua_State *L) {
  const char *f = (const char *)lua_touserdata(L, 1);
  const char *path = (const char *)lua_touserdata(L, 2);
  if (path == NULL) {
    lua_pushstring(L, "path is NULL");
    lua_error(L);
  }

  lua_getglobal(L, "package");
  lua_getfield(L, -1, f);
  const char *cur_path = lua_tostring(L, -1);

  int clen = strlen(cur_path);
  int plen = strlen(path);
  char *new_path = (char *)lua_newuserdata(L, clen + plen + 1);
  memcpy(new_path, cur_path, clen);
  new_path[clen] = ';';
  memcpy(new_path + clen + 1, path, plen);
  lua_pushstring(L, new_path);

  lua_setfield(L, -4, f);

  return 0;
}


// [-0, +0, -]
int
lbind_append_lua_package_field(lua_State *L, const char *field,
                               const char *path) {
  lua_pushcfunction(L, lcf_append_lua_package_field);
  lua_pushlightuserdata(L, (void *)field);
  lua_pushlightuserdata(L, (void *)path);

  int ret = lua_pcall(L, 2, 0, 0);
  if (ret != LUA_OK) {
    lb_pop_error_msg(L);
  }
  return ret;
}


// [-1, +1, e]
static int
lcf_new_fd_lthread(lua_State *L) {
  int fd = lua_tointeger(L, -1);
  char k[] = {0, 0, 0, 0, 0};
  *((int *)k) = fd;

  lbind_checkmetatable(L, LRK_THREAD_TABLE,
                       "thread table not exist");
  lbind_checkmetatable(L, LRK_THREAD_LOCAL_VAR_TABLE,
                       "thread local var table not exist");
  lua_newthread(L);

  lua_pushstring(L, k);
  lua_pushvalue(L, -2);
  lua_settable(L, -5);

  lua_pushvalue(L, -1);
  lua_newtable(L);

  lua_pushstring(L, "fd");
  lua_pushinteger(L, fd);
  lua_settable(L, -3);

  lua_settable(L, -4);

  return 1;
}


// [-0, +0, -]
lua_State *
lbind_new_fd_lthread(lua_State *L, int fd) {
  lua_pushcfunction(L, lcf_new_fd_lthread);
  lua_pushinteger(L, fd);

  int ret = lua_pcall(L, 1, 1, 0);
  if (ret == LUA_OK) {
    lua_State *T = lua_tothread(L, -1);
    lua_pop(L, 1);
    return T;
  } else {
    lb_pop_error_msg(L);
    return NULL;
  }
}

// [-1, +0, e]
static int
lcf_free_fd_lthread(lua_State *L) {
  int fd = lua_tointeger(L, -1);
  char k[] = {0, 0, 0, 0, 0};
  *((int *)k) = fd;

  lbind_checkmetatable(L, LRK_THREAD_TABLE,
                       "thread table not exist");
  lua_pushstring(L, k);
  lua_pushnil(L);
  lua_settable(L, -3);

  return 0;
}


// [-0, +0, -]
int
lbind_free_fd_lthread(lua_State *L, int fd) {
  lua_pushcfunction(L, lcf_free_fd_lthread);
  lua_pushinteger(L, fd);

  int ret = lua_pcall(L, 1, 0, 0);
  if (ret != LUA_OK) {
    lb_pop_error_msg(L);
  }
  return ret;
}


// [-1, +0, e]
static int
lcf_ref_lcode_chunk(lua_State *L) {
  const char *filename = (const char *)lua_touserdata(L, -1);

  lbind_checkmetatable(L, LRK_LCODE_CHUNK_TABLE,
                       "lcode chunk table not exist");
  lua_pushstring(L, filename);
  luaL_loadfile(L, filename);
  lua_settable(L, -3);

  return 0;
}


// [-0, +0, -]
int
lbind_ref_lcode_chunk(lua_State *L, const char *filename) {
  lua_pushcfunction(L, lcf_ref_lcode_chunk);
  lua_pushlightuserdata(L, (void *)filename);

  int ret = lua_pcall(L, 1, 0, 0);
  if (ret != LUA_OK) {
    lb_pop_error_msg(L);
  }
  return ret;
}


// [-1, +0, e]
static int
lcf_unref_lcode_chunk(lua_State *L) {
  const char *filename = (const char *)lua_touserdata(L, -1);
  lua_pushstring(L, filename);
  lua_pushnil(L);
  lua_settable(L, -3);

  return 0;
}


// [-0, +0, -]
int
lbind_unref_lcode_chunk(lua_State *L, const char *filename) {
  lua_pushcfunction(L, lcf_unref_lcode_chunk);
  lua_pushlightuserdata(L, (void *)filename);

  int ret = lua_pcall(L, 1, 0, 0);
  if (ret != LUA_OK) {
    lb_pop_error_msg(L);
  }
  return ret;
}


// [-1, +1, e]
static int
lcf_get_lcode_chunk(lua_State *L) {
  const char *filename = (const char *)lua_touserdata(L, -1);
  lbind_checkmetatable(L, LRK_LCODE_CHUNK_TABLE,
                       "lcode chunk table not exist");
  lua_pushstring(L, filename);
  lua_gettable(L, -2);

  return 1;
}


// [-0, +1, -]
int
lbind_get_lcode_chunk(lua_State *L, const char *filename) {
  lua_pushcfunction(L, lcf_get_lcode_chunk);
  lua_pushlightuserdata(L, (void *)filename);

  int ret = lua_pcall(L, 1, 1, 0);
  if (ret != LUA_OK) {
    lb_pop_error_msg(L);
  }
  return ret;
}
