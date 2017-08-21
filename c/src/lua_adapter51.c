#include "lua_adapter.h"
#ifdef LUA51

// [-0, +1, e]
void
aluaL_newmetatable_with_methods(lua_State *L, const char *name,
                                const luaL_Reg I[]) {
  if (luaL_getmetatable(L, name) == 1) {
    return;
  }

  luaL_newmetatable(L, name);
  int i = 0;
  while (I[i].name != NULL && I[i].func != NULL) {
    luaL_Reg reg = I[i];
    lua_pushcfunction(L, reg.func);
    lua_setfield(L, -2, reg.name);
    i += 1;
  }
  lua_pushvalue(L, -1);
  lua_setfield(L, -2, "__index");
}

#endif
