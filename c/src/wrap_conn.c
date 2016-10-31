#include <time.h>
#include "utils.h"
#include "wrap_conn.h"

#define node_ut_lt(a, b) (container_of(a, as_rb_conn_t, ut_idx)->utime < \
                          container_of(b, as_rb_conn_t, ut_idx)->utime)
#define node_fd_lt(a, b) (container_of(a, as_rb_conn_t, fd_idx)->fd < \
                          container_of(b, as_rb_conn_t, fd_idx)->fd)


void
rb_conn_pool_init(as_rb_conn_pool_t *p) {
  p->ut_tree.root = NULL;
  p->fd_tree.root = NULL;
}


void
rb_conn_pool_insert(as_rb_conn_pool_t *p, as_rb_conn_t *c) {
  rb_tree_insert(&p->ut_tree, &c->ut_idx, node_ut_lt);
  rb_tree_insert_case(&p->ut_tree, &c->ut_idx);
  rb_tree_insert(&p->fd_tree, &c->fd_idx, node_fd_lt);
  rb_tree_insert_case(&p->fd_tree, &c->fd_idx);
}


void
rb_conn_pool_update_conn_ut(as_rb_conn_pool_t *p, as_rb_conn_t *c) {
  c->utime = time(NULL);
  debug_log(">> %d\n", c->fd);
  rb_tree_delete(&p->ut_tree, &c->ut_idx);
  rb_tree_node_init(&c->ut_idx);
  rb_tree_insert(&p->ut_tree, &c->ut_idx, node_ut_lt);
  rb_tree_insert_case(&p->ut_tree, &c->ut_idx);
}


void
rb_conn_pool_delete(as_rb_conn_pool_t *p, as_rb_conn_t *c) {
  rb_tree_delete(&p->ut_tree, &c->ut_idx);
  rb_tree_delete(&p->fd_tree, &c->fd_idx);
}
