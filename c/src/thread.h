#ifndef __AS_THREAD__
#define __AS_THREAD__

#include <stdint.h>
#include <time.h>
#include "lua_adapter.h"
#include "rb_tree.h"
#include "utils.h"

#define AS_TSTATUS_READY  0x00
#define AS_TSTATUS_SLEEP  0x01
#define AS_TSTATUS_CLOSE  0x02

typedef void (*as_release_thread_res_f)(void *);

typedef struct as_dlist_node_s {
  struct as_dlist_node_s  *next;
  struct as_dlist_node_s  *prev;
} as_dlist_node_t;

typedef struct {
  as_release_thread_res_f *resf;
  as_dlist_node_t         node;
  uint8_t                 d[];
} as_thread_res_t;

typedef struct {
  as_tid_t          tid;
  lua_State         *T;
  time_t            ct;
  time_t            ut;
  time_t            et;
  as_rb_node_t      pidx;
  as_rb_tree_t      *pool;
  int               status;
  int               fd;
  as_thread_res_t   *resl;
} as_thread_t;

extern int
asthread_init(as_thread_t *t, int fd, lua_State *L);

#endif
