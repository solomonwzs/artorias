#ifndef __WRAP_CONN__
#define __WRAP_CONN__

#include <time.h>
#include "rb_tree.h"

typedef struct {
  as_rb_node_t  ut_idx;
  time_t        utime;
  int           fd;
} as_rb_conn_t;

typedef struct {
  as_rb_tree_t ut_tree;
} as_rb_conn_pool_t;

#define empty_rb_conn_pool {NULL}

#define rb_conn_init(_c_, _fd_) do {\
  (_c_)->fd = (_fd_);\
  (_c_)->utime = time(NULL);\
} while (0)

#define rb_conn_pool_init(_p_) do {\
  (_p_)->ut_tree.root = NULL;\
} while (0)

#define rb_conn_pool_delete(_p_, _c_) do {\
  rb_tree_delete(&(_p_)->ut_tree, &(_c_)->ut_idx);\
} while (0)

extern void
rb_conn_pool_insert(as_rb_conn_pool_t *p, as_rb_conn_t *c);

extern void
rb_conn_pool_update_conn_ut(as_rb_conn_pool_t *p, as_rb_conn_t *c);

#endif
