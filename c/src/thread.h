#ifndef __AS_THREAD__
#define __AS_THREAD__

#include <stdint.h>
#include <time.h>
#include "lua_adapter.h"
#include "rb_tree.h"
#include "utils.h"

#define AS_TSTATUS_OK     0x00
#define AS_TSTATUS_CLOSE  0x01

typedef void (*as_release_thread_res_f)(void *);

typedef struct as_thread_res_s {
  as_release_thread_res_f *release;
  struct as_thread_res_s  *next;
  struct as_thread_res_s  *prev;
  uint8_t                 d[];
} as_thread_res_t;

typedef struct {
  as_tid_t        tid;
  lua_State       *T;
  time_t          ct;
  time_t          ut;
  as_rb_node_t    ut_idx;
  int             status;
  int             fd;
  as_thread_res_t *resl;
} as_thread_t;

typedef struct {
  as_rb_tree_t ut_tree;
} as_thread_pool_t;

#endif
