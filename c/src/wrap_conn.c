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
  c->cp = NULL;
  c->T = NULL;

  return 0;
}


void
rb_conn_pool_insert(as_rb_conn_pool_t *p, as_rb_conn_t *c) {
  c->cp = p;
  rb_tree_insert(&p->ut_tree, &c->ut_idx, node_ut_lt);
  rb_tree_insert_case(&p->ut_tree, &c->ut_idx);
}


void
rb_conn_update_ut(as_rb_conn_t *c) {
  time_t now = time(NULL);
  if (now != c->utime) {
    c->utime = time(NULL);

    as_rb_conn_pool_t *p = c->cp;
    if (p != NULL) {
      rb_tree_delete(&p->ut_tree, &c->ut_idx);
      rb_tree_insert(&p->ut_tree, &c->ut_idx, node_ut_lt);
      rb_tree_insert_case(&p->ut_tree, &c->ut_idx);
    }
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
  as_rb_node_t **m;
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
    m = &ut->root;
    while ((*m)->left != NULL &&
           (((*m)->color == BLACK &&
             ((*m)->right == NULL || (*m)->right->color == BLACK)) ||
            now - to_rb_conn((*m)->left)->utime <= secs)) {
      m = &(*m)->left;
    }
    if ((ret = (*m)->left) != NULL) {
      ret->parent = NULL;
      n = (*m);
      *m = (*m)->right;
      if ((*m) != NULL) {
        (*m)->color = BLACK;
        (*m)->parent = n->parent;
        rb_tree_insert_to_most_left(m, n);
        rb_tree_insert_case(ut, n);
      }
    }
    return ret;
  }
}
