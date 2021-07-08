#include "rb_tree.h"

#include "utils.h"

#define is_null_or_black(_n_) ((_n_) == NULL || (_n_)->color == BLACK)

#define delete_case1(_t_, _n_, _fa_, _s_, _lr1_, _lr2_) \
  do {                                                  \
    _s_->color = BLACK;                                 \
    _fa_->color = RED;                                  \
    rotate_##_lr1_(_t_, _fa_);                          \
    _s_ = _fa_->_lr2_;                                  \
  } while (0)

#define delete_case2(_n_, _fa_, _s_) \
  do {                               \
    _s_->color = RED;                \
    _n_ = _fa_;                      \
    _fa_ = _n_->parent;              \
  } while (0)

#define delete_case3(_t_, _n_, _fa_, _s_, _lr1_, _lr2_) \
  do {                                                  \
    if (_s_->_lr1_ != NULL) {                           \
      _s_->_lr1_->color = BLACK;                        \
    }                                                   \
    _s_->color = RED;                                   \
    rotate_##_lr2_(_t_, _s_);                           \
    _s_ = _fa_->_lr2_;                                  \
  } while (0)

#define delete_case4(_t_, _n_, _fa_, _s_, _lr1_, _lr2_) \
  do {                                                  \
    _s_->color = _fa_->color;                           \
    _fa_->color = BLACK;                                \
    if (_s_->_lr2_ != NULL) {                           \
      _s_->_lr2_->color = BLACK;                        \
    }                                                   \
    rotate_##_lr1_(_t_, _fa_);                          \
    _n_ = _t_->root;                                    \
  } while (0)

static inline as_rb_node_t *uncle(as_rb_node_t *n) {
  as_rb_node_t *g = n->parent->parent;
  return n->parent == g->right ? g->left : g->right;
}

static inline void rotate_right(as_rb_tree_t *t, as_rb_node_t *q) {
  /***********************************************************
   *         +---+                          +---+
   *         | q |                          | p |
   *         +---+                          +---+
   *        /     \     right rotation     /     \
   *     +---+   +---+  ------------->  +---+   +---+
   *     | p |   | z |                  | x |   | q |
   *     +---+   +---+                  +---+   +---+
   *    /     \                                /     \
   * +---+   +---+                          +---+   +---+
   * | x |   | y |                          | y |   | z |
   * +---+   +---+                          +---+   +---+
   **********************************************************/
  as_rb_node_t *p = q->left;

  as_rb_node_t *y = p->right;
  if (y != NULL) {
    y->parent = q;
  }
  q->left = y;

  as_rb_node_t *fa = q->parent;
  if (fa == NULL) {
    t->root = p;
  } else {
    if (q == fa->left) {
      fa->left = p;
    } else {
      fa->right = p;
    }
  }
  p->right = q;
  p->parent = fa;
  q->parent = p;
}

static inline void rotate_left(as_rb_tree_t *t, as_rb_node_t *p) {
  /***********************************************************
   *         +---+                          +---+
   *         | q |                          | p |
   *         +---+                          +---+
   *        /     \                        /     \
   *     +---+   +---+                  +---+   +---+
   *     | p |   | z |                  | x |   | q |
   *     +---+   +---+  <-------------  +---+   +---+
   *    /     \          left rotation         /     \
   * +---+   +---+                          +---+   +---+
   * | x |   | y |                          | y |   | z |
   * +---+   +---+                          +---+   +---+
   **********************************************************/
  as_rb_node_t *q = p->right;
  if (q == NULL) {
    return;
  }

  as_rb_node_t *y = q->left;
  if (y != NULL) {
    y->parent = p;
  }
  p->right = y;

  as_rb_node_t *fa = p->parent;
  if (fa == NULL) {
    t->root = q;
  } else {
    if (p == fa->left) {
      fa->left = q;
    } else {
      fa->right = q;
    }
  }
  q->left = p;
  q->parent = fa;
  p->parent = q;
}

static inline void rb_tree_delete_case(as_rb_tree_t *t, as_rb_node_t *n,
                                       as_rb_node_t *fa) {
  as_rb_node_t *s;
  while ((n == NULL || n->color == BLACK) && n != t->root) {
    if (n == fa->left) {
      s = fa->right;
      if (s->color == RED) {
        delete_case1(t, n, fa, s, left, right);
      }
      if (is_null_or_black(s->left) && is_null_or_black(s->right)) {
        delete_case2(n, fa, s);
      } else {
        if (is_null_or_black(s->right)) {
          delete_case3(t, n, fa, s, left, right);
        }
        delete_case4(t, n, fa, s, left, right);
        break;
      }
    } else {
      s = fa->left;
      if (s->color == RED) {
        delete_case1(t, n, fa, s, right, left);
      }
      if (is_null_or_black(s->left) && is_null_or_black(s->right)) {
        delete_case2(n, fa, s);
      } else {
        if (is_null_or_black(s->left)) {
          delete_case3(t, n, fa, s, right, left);
        }
        delete_case4(t, n, fa, s, right, left);
        break;
      }
    }
  }
  if (n != NULL) {
    n->color = BLACK;
  }
}

static void inline swap_node_with_leaf(as_rb_tree_t *t, as_rb_node_t *n,
                                       as_rb_node_t *leaf) {
  as_rb_node_t *fa;
  as_rb_node_t *rc;
  char color = leaf->color;

  if ((fa = n->parent) == NULL) {
    t->root = leaf;
  } else {
    if (n == fa->left) {
      fa->left = leaf;
    } else {
      fa->right = leaf;
    }
  }

  rc = leaf->right;
  fa = leaf->parent;
  if (fa == n) {
    fa = leaf;
  } else {
    if (rc != NULL) {
      rc->parent = fa;
    }
    fa->left = rc;
    leaf->right = n->right;
    n->right->parent = leaf;
  }

  leaf->parent = n->parent;
  leaf->color = n->color;
  leaf->left = n->left;
  n->left->parent = leaf;

  if (color == BLACK) {
    rb_tree_delete_case(t, rc, fa);
  }
}

static void inline swap_node_with_subtree(as_rb_tree_t *t, as_rb_node_t *n,
                                          as_rb_node_t *sub) {
  as_rb_node_t *gf = n->parent;

  if (gf == NULL) {
    t->root = sub;
    sub->color = BLACK;
    sub->parent = NULL;
  } else {
    if (n == gf->left) {
      gf->left = sub;
    } else {
      gf->right = sub;
    }
    sub->parent = gf;

    if (n->color == RED || sub->color == RED) {
      sub->color = BLACK;
    } else {
      rb_tree_delete_case(t, sub, gf);
    }
  }
}

static void inline remove_node_has_one_child(as_rb_tree_t *t,
                                             as_rb_node_t *ori) {
  as_rb_node_t *fa = ori->parent;
  as_rb_node_t *sub = ori->left == NULL ? ori->right : ori->left;
  if (sub != NULL) {
    sub->parent = fa;
  }
  if (fa != NULL) {
    if (ori == fa->left) {
      fa->left = sub;
    } else {
      fa->right = sub;
    }
  } else {
    t->root = sub;
  }
  if (ori->color == BLACK) {
    rb_tree_delete_case(t, sub, fa);
  }
}

void rb_tree_delete(as_rb_tree_t *t, as_rb_node_t *n) {
  if (t == NULL && n == NULL) {
    return;
  }

  if (n->left == NULL || n->right == NULL) {
    remove_node_has_one_child(t, n);
  } else {
    as_rb_node_t *leaf = n->right;
    while (leaf->left != NULL) {
      leaf = leaf->left;
    }
    swap_node_with_leaf(t, n, leaf);
  }
}

void rb_tree_insert_case(as_rb_tree_t *t, as_rb_node_t *n) {
  if (t == NULL || n == NULL) {
    return;
  }

  as_rb_node_t *fa;
  as_rb_node_t *gf;
  as_rb_node_t *uc;
  while ((fa = n->parent) && fa->color == RED) {
    if ((uc = uncle(n)) && uc->color == RED) {
      fa->color = BLACK;
      uc->color = BLACK;

      gf = fa->parent;
      gf->color = RED;
      n = gf;

      continue;
    }

    gf = fa->parent;
    if (n == fa->right && fa == gf->left) {
      rotate_left(t, fa);
      n = n->left;

      fa = n->parent;
      gf = fa->parent;
    } else if (n == fa->left && fa == gf->right) {
      rotate_right(t, fa);
      n = n->right;

      fa = n->parent;
      gf = fa->parent;
    }

    fa->color = BLACK;
    gf->color = RED;
    if (n == fa->left) {
      rotate_right(t, gf);
    } else {
      rotate_left(t, gf);
    }
  }
  t->root->color = BLACK;
}

void rb_tree_insert_to_most_left(as_rb_node_t **root, as_rb_node_t *n) {
  n->parent = n->left = n->right = NULL;
  n->color = RED;
  as_rb_node_t **ptr = root;
  while (*ptr != NULL) {
    n->parent = *ptr;
    ptr = &(*ptr)->left;
  }
  *ptr = n;
}

void rb_tree_insert_to_most_right(as_rb_node_t **root, as_rb_node_t *n) {
  n->parent = n->left = n->right = NULL;
  n->color = RED;
  as_rb_node_t **ptr = root;
  while (*ptr != NULL) {
    n->parent = *ptr;
    ptr = &(*ptr)->right;
  }
  *ptr = n;
}

void rb_tree_remove_subtree(as_rb_tree_t *tree, as_rb_node_t *sub) {
  as_rb_node_t *fa = sub->parent;
  if (fa == NULL) {
    tree->root = NULL;
  } else if (sub == fa->left) {
    as_rb_node_t *sil = fa->right;
    swap_node_with_subtree(tree, fa, sil);
    rb_tree_insert_to_most_left(&sil, fa);
    rb_tree_insert_case(tree, fa);
  } else {
    as_rb_node_t *sil = fa->left;
    swap_node_with_subtree(tree, fa, sil);
    rb_tree_insert_to_most_right(&sil, fa);
    rb_tree_insert_case(tree, fa);
  }
}
