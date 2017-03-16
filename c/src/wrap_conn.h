#ifndef __WRAP_CONN_H__
#define __WRAP_CONN_H__

#include <time.h>
#include <lua.h>
#include <stdio.h>
#include "rb_tree.h"

typedef void (*as_rb_conn_free_wdata_f)(void *data);
typedef ssize_t (*as_rb_conn_write_data_f)(void *data, size_t size);

typedef struct {
} as_rb_conn_write_stack;

typedef struct {
  as_rb_node_t  ut_idx;
  time_t        utime;
  int           fd;
  lua_State     *T;

  void                    *w_data;
  as_rb_conn_write_data_f w_write;
  as_rb_conn_free_wdata_f w_free;
} as_rb_conn_t;

typedef struct {
  as_rb_tree_t ut_tree;
} as_rb_conn_pool_t;

#define NULL_RB_CONN_POOL {NULL}

#define rb_conn_pool_init(_p_) \
    (_p_)->ut_tree.root = NULL

#define rb_conn_pool_delete(_p_, _wc_) \
    rb_tree_delete(&(_p_)->ut_tree, &(_wc_)->ut_idx)

#define rb_conn_set_wfunc(_wc_, _d_, _write_, _free_) do {\
  (_wc_)->w_data = (_d_);\
  (_wc_)->w_write = (_write_);\
  (_wc_)->w_free = (_free_);\
} while (0)\

extern int
rb_conn_init(lua_State *L, as_rb_conn_t *wc, int fd);

extern int
rb_conn_close(as_rb_conn_t *wc);

extern void
rb_conn_pool_insert(as_rb_conn_pool_t *p, as_rb_conn_t *wc);

extern void
rb_conn_pool_update_conn_ut(as_rb_conn_pool_t *p, as_rb_conn_t *wc);

extern as_rb_node_t *
rb_conn_remove_timeout_conn(as_rb_conn_pool_t *p, unsigned secs);

#endif
