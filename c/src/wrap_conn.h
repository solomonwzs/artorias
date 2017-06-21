#ifndef __WRAP_CONN_H__
#define __WRAP_CONN_H__

#include <time.h>
#include <stdio.h>
#include "rb_tree.h"
#include "lua_utils.h"

#define AS_WC_RB_CONN   0x00
#define AS_WC_SL_CONN   0x01

typedef struct {
  uint32_t  type;
  uint8_t   d[];
} as_wrap_conn_t;

typedef struct {
  as_rb_tree_t ut_tree;
} as_rb_conn_pool_t;

typedef struct {
  as_rb_node_t  ut_idx;
  time_t        utime;
  int           fd;
  lua_State     *T;
} as_rb_conn_t;

typedef struct {
  int       fd;
  lua_State *T;
} as_sl_conn_t;

#define sizeof_wrap_conn(_type_) \
    (offsetof(as_wrap_conn_t, d) + sizeof(_type_))

#define rb_conn_pool_init(_p_) \
    (_p_)->ut_tree.root = NULL

#define rb_conn_pool_delete(_cp_, _wc_) \
    rb_tree_delete(&(_cp_)->ut_tree, &(_wc_)->ut_idx)

extern int
rb_conn_init(as_rb_conn_t *wc, int fd);

extern void
rb_conn_pool_insert(as_rb_conn_pool_t *p, as_rb_conn_t *wc);

extern void
rb_conn_update_ut(as_rb_conn_pool_t *p, as_rb_conn_t *wc);

extern as_rb_node_t *
rb_conn_remove_timeout_conn(as_rb_conn_pool_t *p, unsigned secs);

#endif
