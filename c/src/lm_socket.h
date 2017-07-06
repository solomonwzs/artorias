#ifndef __LUA_MODULE_SOCKET__
#define __LUA_MODULE_SOCKET__

#include "thread.h"

#define LM_SOCK_TYPE_SYSTEM   0x00
#define LM_SOCK_TYPE_CUSTOM   0x01

typedef struct {
  as_thread_res_t *res;
  int             type;
} as_lm_socket_t;

#endif
