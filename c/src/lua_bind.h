#ifndef __LUA_BIND__
#define __LUA_BIND__

#include "lua_utils.h"
#include "mem_pool.h"


// [-0, +1, e]
#define lbind_checkmetatable(_L_, _tname_, _emsg_) do {\
  luaL_getmetatable(_L_, _tname_);\
  if (lua_isnil(_L_, -1)) { \
    lua_pushstring(_L_, _emsg_);\
    lua_error(_L_);\
  }\
} while (0)

extern void
lbind_check_metatable_elem_by_tname(lua_State *L, const char *tname);

extern void
lbind_check_metatable_elem(lua_State *L, int idx);

extern int
lbind_dofile(lua_State *L, const char *filename);

extern int
lbind_init_state(lua_State *L);

extern lua_State *
lbind_new_state(as_mem_pool_fixed_t *mp);

extern int
lbind_append_lua_package_field(lua_State *L, const char *field,
                               const char *path);

extern lua_State *
lbind_new_fd_lthread(lua_State *L, int fd);

extern lua_State *
lbind_ref_tid_lthread(lua_State *L, as_tid_t tid);

extern int
lbind_unref_tid_lthread(lua_State *L, as_tid_t tid);

extern int
lbind_ref_fd_lthread(lua_State *L, int fd);

extern int
lbind_unref_fd_lthread(lua_State *T, int fd);

extern int
lbind_ref_lcode_chunk(lua_State *L, const char *filename);

extern int
lbind_unref_lcode_chunk(lua_State *L, const char *filename);

extern int
lbind_get_lcode_chunk(lua_State *L, const char *filename);

extern int
lbind_set_thread_local_var_ptr(lua_State *T, const char *field, void *value);

#define lbind_append_lua_cpath(_L_, _p_) \
    lbind_append_lua_package_field(_L_, "cpath", _p_);

#define lbind_append_lua_path(_L_, _p_) \
    lbind_append_lua_package_field(_L_, "path", _p_);

extern int
lbind_reg_value_int(lua_State *L, const char *field, int value);

extern int
lbind_reg_value_ptr(lua_State *L, const char *field, void *value);

extern void
lbind_scan_stack_elem(lua_State *L);

#endif
