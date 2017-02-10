#ifndef __LUA_UTILS__
#define __LUA_UTILS__

#include <lua.h>
#include "utils.h"

#define LM_COUNTER "as_lm_000"

#define LRK_THREAD_TABLE            "as_lrk_000"
#define LRK_THREAD_LOCAL_VAR_TABLE  "as_lrk_001"

#ifdef DEBUG
#   define lb_pop_error_msg(_L_) do {\
      debug_log("lua error stack trackback: %s\n", lua_tostring(_L_, -1));\
      lutils_traceback(_L_);\
      lua_pop(_L_, 1);\
    } while (0)
#else
#   define lb_pop_error_msg(_L_) lua_pop(_L_, 1)
#endif

extern void
lutils_traceback(lua_State *L);

#endif
