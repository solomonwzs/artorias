#ifndef __LUA_UTILS__
#define __LUA_UTILS__

#include "utils.h"

#define lb_error_msg(_L_) do {\
  debug_log("lua_error: %s\n", lua_tostring(_L_, -1));\
  lua_pop(_L_, 1);\
} while (0)

#define pop_pcall_rcode(_L_, _r_) ({\
  int __r = (_r_);\
  if ((_r_) != LUA_OK) {\
    lb_error_msg(_L_);\
  } else {\
    __r = lua_tointeger(_L_, -1);\
    lua_pop(_L_, 1);\
  }\
  __r;\
})

#endif
