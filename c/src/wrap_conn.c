#include <time.h>
#include "utils.h"
#include "wrap_conn.h"

#define node_ut_lt(a, b) (container_of(a, as_rb_conn_t, ut_idx)->utime < \
                          container_of(b, as_rb_conn_t, ut_idx)->utime)
#define node_fd_lt(a, b) (container_of(a, as_rb_conn_t, fd_idx)->fd < \
                          container_of(b, as_rb_conn_t, fd_idx)->fd)


void
rb_conn_pool_insert(as_rb_conn_pool_t *p, as_rb_conn_t *c) {
  rb_tree_insert(&p->ut_tree, &c->ut_idx, node_ut_lt);
  rb_tree_insert_case(&p->ut_tree, &c->ut_idx);
}


void
rb_conn_pool_update_conn_ut(as_rb_conn_pool_t *p, as_rb_conn_t *c) {
  c->utime = time(NULL);
  rb_tree_delete(&p->ut_tree, &c->ut_idx);
  rb_tree_node_init(&c->ut_idx);
  rb_tree_insert(&p->ut_tree, &c->ut_idx, node_ut_lt);
  rb_tree_insert_case(&p->ut_tree, &c->ut_idx);
}


as_rb_node_t *
rb_conn_remove_timeout_conn(as_rb_conn_pool_t *p, unsigned secs) {
  time_t now = time(NULL);
  as_rb_tree_t *ut = &p->ut_tree;
  as_rb_node_t *n = ut->root;
  as_rb_conn_t *wc = container_of(n, as_rb_conn_t, ut_idx);

  if (now - wc->utime > secs) {
    ut->root = NULL;
    return n;
  }
  return NULL;
}
