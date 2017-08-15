#ifndef __LUA_BIND__
#define __LUA_BIND__

#include "lua_utils.h"
#include "mem_pool.h"

#define LTYPE_INT   0x00
#define LTYPE_PTR   0x01
#define LTYPE_STACK 0x02

#define LTTYPE_THREAD_LOCAL_VAR 0x00
#define LTTYPE_REGISTRY         0x01

// [-0, +1, e]
#define lbind_checkmetatable(_L_, _tname_, _emsg_) do {\
  luaL_getmetatable(_L_, _tname_);\
  if (lua_isnil(_L_, -1)) { \
    lua_pushstring(_L_, _emsg_);\
    lua_error(_L_);\
  }\
} while (0)

// [-0, +1, e]
#define lbind_checkregvalue(_L_, _field_, _type_, _emsg_) do {\
  lua_pushstring(_L_, _field_);\
  if (lua_gettable(_L_, LUA_REGISTRYINDEX) != (_type_)) {\
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
_lbind_append_lua_package_field(lua_State *L, const char *field,
                                const char *path);

extern int
_lbind_set_va_list(lua_State *L, int ttype, int n, ...);

extern lua_State *
lbind_ref_tid_lthread(lua_State *L, as_tid_t tid);

extern int
lbind_unref_tid_lthread(lua_State *L, as_tid_t tid);

extern int
lbind_ref_lcode_chunk(lua_State *L, const char *filename);

extern int
lbind_unref_lcode_chunk(lua_State *L, const char *filename);

extern int
lbind_get_lcode_chunk(lua_State *L, const char *filename);

#define lbind_set_thread_local_vars(_T_, _n_, ...) \
    _lbind_set_va_list(_T_, LTTYPE_THREAD_LOCAL_VAR, _n_, ## __VA_ARGS__)

extern int
lbind_get_thread_local_vars(lua_State *T, int n, ...);

#define lbind_append_lua_cpath(_L_, _p_) \
    _lbind_append_lua_package_field(_L_, "cpath", _p_);

#define lbind_append_lua_path(_L_, _p_) \
    _lbind_append_lua_package_field(_L_, "path", _p_);

#define lbind_reg_values(_T_, _n_, ...) \
    _lbind_set_va_list(_T_, LTTYPE_REGISTRY, _n_, ## __VA_ARGS__)

extern void
_lbind_scan_stack_elem(lua_State *L);

#define lbind_scan_stack_elem(_L_) do {\
  debug_log("stack: %p(%d)\n", _L_, lua_gettop(_L_));\
  _lbind_scan_stack_elem(_L_);\
} while (0)

#endif
