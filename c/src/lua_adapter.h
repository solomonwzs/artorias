#ifndef __LUA_ADAPTER__
#define __LUA_ADAPTER__

// #define LUA51
#ifdef LUA51
#   include <luajit-2.0/lua.h>
#   include <luajit-2.0/lualib.h>
#   include <luajit-2.0/lauxlib.h>

#   define LUA_OK 0
#   define lua_isinteger(_L_, _i_) lua_isnumber(_L_, _i_)
#   define alua_resume(_L_, _n_) lua_resume(_L_, _n_)
#   define aluaL_newlib(_L_, _n_, _I_) luaL_register(_L_, _n_, _I_)
#else
#   include <lua.h>
#   include <lualib.h>
#   include <lauxlib.h>

#   define alua_resume(_L_, _n_) lua_resume(_L_, NULL, _n_)
#   define aluaL_newlib(_L_, _n_, _I_) luaL_newlib(_L_, _I_)
#endif

extern void
aluaL_newmetatable_with_methods(lua_State *L, const char *name,
                                const luaL_Reg I[]);

#endif
