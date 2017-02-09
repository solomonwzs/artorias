#ifndef __WRAP_CONN__
#define __WRAP_CONN__

#include <time.h>
#include <lua.h>
#include "rb_tree.h"

typedef struct {
  as_rb_node_t  ut_idx;
  time_t        utime;
  int           fd;
  lua_State     *T;
} as_rb_conn_t;

typedef struct {
  as_rb_tree_t ut_tree;
} as_rb_conn_pool_t;

#define NULL_RB_CONN_POOL {NULL}

#define rb_conn_pool_init(_p_) \
    (_p_)->ut_tree.root = NULL

#define rb_conn_pool_delete(_p_, _wc_) \
    rb_tree_delete(&(_p_)->ut_tree, &(_wc_)->ut_idx)

extern int
rb_conn_init(lua_State *L, as_rb_conn_t *wc, int fd);

extern int
rb_conn_close(lua_State *L, as_rb_conn_t *wc);

extern void
rb_conn_pool_insert(as_rb_conn_pool_t *p, as_rb_conn_t *wc);

extern void
rb_conn_pool_update_conn_ut(as_rb_conn_pool_t *p, as_rb_conn_t *wc);

extern as_rb_node_t *
rb_conn_remove_timeout_conn(as_rb_conn_pool_t *p, unsigned secs);

#endif
