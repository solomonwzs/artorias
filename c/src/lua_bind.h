#ifndef __LUA_BIND__
#define __LUA_BIND__

#include <lua.h>
#include "mem_pool.h"

extern int
lbind_dofile(lua_State *L, const char *filename);

extern int
lbind_init_state(lua_State *L, as_mem_pool_fixed_t *mp);

extern lua_State *
lbind_new_state(as_mem_pool_fixed_t *mp);

extern int
lbind_append_lua_package_field(lua_State *L, const char *field,
                               const char *path);

extern lua_State *
lbind_new_fd_lthread(lua_State *L, int fd);

extern int
lbind_free_fd_lthread(lua_State *L, int fd);

extern int
lbind_ref_lcode_chunk(lua_State *L, const char *filename);

extern int
lbind_unref_lcode_chunk(lua_State *L, const char *filename);

#define lbind_append_lua_cpath(_L_, _p_) \
    lbind_append_lua_package_field(_L_, "cpath", _p_);

#define lbind_append_lua_path(_L_, _p_) \
    lbind_append_lua_package_field(_L_, "path", _p_);

#endif
