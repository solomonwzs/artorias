#ifndef __LUA_UTILS__
#define __LUA_UTILS__

#include "lua_adapter.h"
#include "utils.h"

#define LM_COUNTER   "as_lm_000"
#define LM_TSOCKET   "as_lm_001"
#define LM_SOCKET    "as_lm_002"
#define LM_BYTES     "as_lm_003"
#define LM_BYTES_REF "as_lm_004"

#define LRK_WORKER_CTX             "as_lrk_000"
#define LRK_FD_THREAD_TABLE        "as_lrk_001"
#define LRK_THREAD_LOCAL_VAR_TABLE "as_lrk_002"
#define LRK_LCODE_CHUNK_TABLE      "as_lrk_003"
#define LRK_TID_THREAD_TABLE       "as_lrk_004"
#define LRK_SERVER_EPFD            "as_lrk_005"
#define LRK_MEM_POOL               "as_lrk_006"
#define LRK_BYTES_REF_TABLE        "as_lrk_007"

#define LLK_RES_SOCK_TABLE "as_llk_000"
#define LLK_THREAD         "as_llk_001"

#define LAS_D_TIMOUT_SECS 15

#define LAS_S_WAIT_FOR_INPUT    0x00
#define LAS_S_WAIT_FOR_OUTPUT   0x01
#define LAS_S_READY_TO_INPUT    0x02
#define LAS_S_READY_TO_OUTPUT   0x03
#define LAS_S_SOCKET_CLOSEED    0x04
#define LAS_S_YIELD_FOR_IO      0x05
#define LAS_S_YIELD_FOR_SLEEP   0x06
#define LAS_S_YIELD_FOR_EV      0x07
#define LAS_S_RESUME_IO         0x08
#define LAS_S_RESUME_IO_ERROR   0x09
#define LAS_S_RESUME_IO_TIMEOUT 0x0a
#define LAS_S_RESUME_SLEEP      0x0b

#ifdef DEBUG
#define lb_pop_error_msg(_L_)                                            \
  do {                                                                   \
    debug_log("lua error stack trackback: %s\n", lua_tostring(_L_, -1)); \
    lutils_traceback(_L_);                                               \
    lua_pop(_L_, 1);                                                     \
  } while (0)
#else
#define lb_pop_error_msg(_L_) lua_pop(_L_, 1)
#endif

extern void lutils_traceback(lua_State *L);

#endif
