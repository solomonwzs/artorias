#ifndef __WRAP_CONN__
#define __WRAP_CONN__

#include <time.h>
#include "rb_tree.h"

typedef struct {
  as_rb_node_t  ct_idx;
  as_rb_node_t  fd_idx;
  time_t        ctime;
  int           fd;
} as_rb_conn_t;

typedef struct {
  as_rb_tree_t ct_tree;
} as_rb_conn_pool_t;

#define rb_conn_init(c, fd) do {\
  rb_tree_node_init(&c->ct_idx);\
  c->fd = (fd);\
  c->ctime = time(NULL);\
} while (0)

#endif
