#ifndef __LUA_MODULE_SOCKET__
#define __LUA_MODULE_SOCKET__

#include "wrap_conn.h"

typedef struct {
  as_rb_conn_t  *conn;
  int           ot_secs;
} as_lm_socket_t;

#endif
