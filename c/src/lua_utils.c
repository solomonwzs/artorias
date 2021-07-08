#include "lua_utils.h"

void lutils_traceback(lua_State *L) {
  lua_Debug ar;
  int n = 0;
  while (lua_getstack(L, n, &ar)) {
    lua_getinfo(L, "Sln", &ar);
    if (ar.name) {
      fprintf(stderr, "\tstack[%d] -> line %d: %s()[%s:%d]\n", n,
              ar.currentline, ar.name, ar.short_src, ar.linedefined);
    } else {
      fprintf(stderr, "\tstack[%d] -> line %d: Unknown[%s:%d]\n", n,
              ar.currentline, ar.short_src, ar.linedefined);
    }
    n += 1;
  }
}
