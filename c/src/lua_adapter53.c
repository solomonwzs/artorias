#include "lua_adapter.h"
#ifndef LUA51

// [-0, +1, e]
void aluaL_newmetatable_with_methods(lua_State *L, const char *name,
                                     const luaL_Reg I[]) {
  if (luaL_getmetatable(L, name) == 1) {
    return;
  }

  luaL_newmetatable(L, name);
  lua_pushvalue(L, -1);
  lua_setfield(L, -2, "__index");
  luaL_setfuncs(L, I, 0);
}

#endif
