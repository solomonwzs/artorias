#include "utils.h"
#include "wrap_conn.h"

#define node_ct_comp(a, b) ((a)->ctime < (b)->ctime)

void
rb_conn_pool_init(as_rb_conn_pool_t *p) {
  p->ct_tree.root = NULL;
}


void
rb_conn_pool_insert(as_rb_conn_pool_t *p, as_rb_conn_t *c) {
}
