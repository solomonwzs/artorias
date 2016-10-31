#ifndef __WRAP_CONN__
#define __WRAP_CONN__

#include <time.h>
#include "rb_tree.h"

typedef struct {
  as_rb_node_t  ut_idx;
  as_rb_node_t  fd_idx;
  time_t        utime;
  int           fd;
} as_rb_conn_t;

typedef struct {
  as_rb_tree_t ut_tree;
  as_rb_tree_t fd_tree;
} as_rb_conn_pool_t;

#define rb_conn_init(_c_, _fd_) do {\
  rb_tree_node_init(&(_c_)->ut_idx);\
  rb_tree_node_init(&(_c_)->fd_idx);\
  (_c_)->fd = (_fd_);\
  (_c_)->utime = time(NULL);\
} while (0)

extern void
rb_conn_pool_init(as_rb_conn_pool_t *p);

extern void
rb_conn_pool_insert(as_rb_conn_pool_t *p, as_rb_conn_t *c);

extern void
rb_conn_pool_update_conn_ut(as_rb_conn_pool_t *p, as_rb_conn_t *c);

extern void
rb_conn_pool_delete(as_rb_conn_pool_t *p, as_rb_conn_t *c);

#endif
