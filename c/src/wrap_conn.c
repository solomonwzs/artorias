#include "utils.h"
#include "lua_bind.h"
#include "wrap_conn.h"

#define node_ut_lt(a, b) (container_of(a, as_rb_conn_t, ut_idx)->utime < \
                          container_of(b, as_rb_conn_t, ut_idx)->utime)
#define to_rb_conn(_n_) \
    ((as_rb_conn_t *)(container_of(_n_, as_rb_conn_t, ut_idx)))


int
rb_conn_init(as_rb_conn_t *c, int fd) {
  c->fd = fd;
  c->utime = time(NULL);
  c->T = NULL;

  return 0;
}


void
rb_conn_pool_insert(as_rb_conn_pool_t *p, as_rb_conn_t *c) {
  rb_tree_insert(&p->ut_tree, &c->ut_idx, node_ut_lt);
  rb_tree_insert_case(&p->ut_tree, &c->ut_idx);
}


void
rb_conn_update_ut(as_rb_conn_pool_t *p, as_rb_conn_t *c) {
  time_t now = time(NULL);
  if (now != c->utime) {
    c->utime = now;

    rb_tree_delete(&p->ut_tree, &c->ut_idx);
    rb_tree_insert(&p->ut_tree, &c->ut_idx, node_ut_lt);
    rb_tree_insert_case(&p->ut_tree, &c->ut_idx);
  }
}


as_rb_node_t *
rb_conn_remove_timeout_conn(as_rb_conn_pool_t *p, unsigned secs) {
  as_rb_tree_t *ut = &p->ut_tree;
  if (ut->root == NULL) {
    return NULL;
  }

  time_t now = time(NULL);
  as_rb_node_t *ret;
  as_rb_node_t *n;
  if (now - to_rb_conn(ut->root)->utime > secs) {
    ret = n = ut->root;
    while (n->right != NULL && now - to_rb_conn(n->right)->utime > secs) {
      n = n->right;
    }
    ut->root = n->right;
    if (n->right != NULL) {
      n->right->parent = NULL;
      n->right->color = BLACK;
    }
    n->right = NULL;
    return ret;
  } else {
    ret = ut->root->left;
    while (ret != NULL && now - to_rb_conn(ret)->utime <= secs) {
      ret = ret->left;
    }

    if (ret != NULL) {
      rb_tree_remove_subtree(ut, ret);
    }
    return ret;
  }
}
