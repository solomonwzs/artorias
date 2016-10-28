#include "rb_tree.h"

#define is_null_or_black(n) ((n) == NULL || (n)->color == BLACK)

#define delete_case1(t, n, fa, s, lr1, lr2) do {\
  s->color = BLACK;\
  fa->color = RED;\
  rotate_##lr1(t, fa);\
  s = fa->lr2;\
} while (0)

#define delete_case2(n, fa, s) do {\
  s->color = RED;\
  n = fa;\
  fa = n->parent;\
} while (0)

#define delete_case3(t, n, fa, s, lr1, lr2) do {\
  if (s->lr2 != NULL) {\
    s->lr2->color = BLACK;\
  }\
  s->color = RED;\
  rotate_##lr1(t, s);\
  s = fa->lr1;\
} while(0)\

#define delete_case4(t, n, fa, s, lr1, lr2) do {\
  s->color = fa->color;\
  fa->color = BLACK;\
  if (s->lr1 != NULL) {\
    s->lr1->color = BLACK;\
  }\
  rotate_##lr2(t, fa);\
  n = t->root;\
} while(0)


static inline as_rb_node_t *
grandparent(as_rb_node_t *n) {
  if (n == NULL || n->parent == NULL) {
    return NULL;
  }
  return n->parent->parent;
}


static inline as_rb_node_t *
uncle(as_rb_node_t *n) {
  as_rb_node_t *g = grandparent(n);
  if (g == NULL) {
    return NULL;
  }
  return n->parent == g->right ? g->left : g->right;
}


static inline void
rotate_right(as_rb_tree_t *t, as_rb_node_t *q) {
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


static inline void
rotate_left(as_rb_tree_t *t, as_rb_node_t *p) {
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


static inline void
rb_tree_delete_case(as_rb_tree_t *t, as_rb_node_t *n, as_rb_node_t *fa) {
  as_rb_node_t *s;
  while ((n == NULL || n->color == BLACK) && n != t->root) {
    if (n == fa->left) {
      s = fa->right;
      if (s->color == RED) {
        delete_case1(t, n, fa, s, left, right);
      }
      if (is_null_or_black(s->left) ||
          is_null_or_black(s->right)) {
        delete_case2(n, fa, s);
      } else {
        if (is_null_or_black(s->right)) {
          delete_case3(t, n, fa, s, right, left);
        }
        delete_case4(t, n, s, fa, right, left);
        break;
      }
    } else {
      s = fa->left;
      if (s->color == RED) {
        delete_case1(t, n, fa, s, right, left);
      }
      if (is_null_or_black(s->left) ||
          is_null_or_black(s->right)) {
        delete_case2(n, fa, s);
      } else {
        if (is_null_or_black(s->left)) {
          delete_case3(t, n, fa, s, left, right);
        }
        delete_case4(t, n, s, fa, left, right);
        break;
      }
    }
  }
  if (n != NULL) {
    n->color = BLACK;
  }
}


static void inline
swap_and_remove_ori_node(as_rb_tree_t *t, as_rb_node_t *ori, as_rb_node_t *sub) {
  as_rb_node_t *fa;
  as_rb_node_t *rc;
  char color = sub->color;

  if ((fa = ori->parent) == NULL) {
    t->root = sub;
  } else {
    if (ori == fa->left) {
      fa->left = sub;
    } else {
      fa->right = sub;
    }
  }

  rc = sub->right;
  fa = sub->parent;
  if (fa == ori) {
    fa = sub;
  } else {
    if (rc != NULL) {
      rc->parent = fa;
    }
    fa->left = rc;
    sub->right = ori->right;
    ori->right->parent = sub;
  }

  sub->parent = ori->parent;
  sub->color = ori->color;
  sub->left = ori->left;
  ori->left->parent = sub;

  if (color == BLACK) {
    rb_tree_delete_case(t, rc, fa);
  }
}


static void inline
remove_node_has_one_child(as_rb_tree_t *t, as_rb_node_t *ori) {
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


void
rb_tree_delete(as_rb_tree_t *t, as_rb_node_t *n) {
  if (t == NULL && n == NULL) {
    return;
  }

  if (n->left == NULL || n->right == NULL) {
    remove_node_has_one_child(t, n);
  } else {
    as_rb_node_t *ori = n;
    as_rb_node_t *left;

    n = n->right;
    while ((left = n->left) != NULL) {
      n = n->left;
    }
    swap_and_remove_ori_node(t, ori, n);
  }
}


void
rb_tree_insert_case(as_rb_tree_t *t, as_rb_node_t *n) {
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


void
rb_tree_postorder_travel(as_rb_tree_t *t, void *data,
                         void (*func)(as_rb_node_t *, void *)) {
  if (t == NULL) {
    return;
  }

  as_rb_node_t *tn = t->root;
  while (tn != NULL) {
    if (tn->left != NULL) {
      tn = tn->left;
    } else if (tn->right != NULL) {
      tn = tn->right;
    } else {
      as_rb_node_t *fa = tn->parent;
      if (fa != NULL) {
        if (tn == fa->left) {
          fa->left = NULL;
        } else {
          fa->right = NULL;
        }
      }
      func(tn, data);
      tn = fa;
    }
  }
}

void
rb_tree_delete_subtree(as_rb_tree_t *t, as_rb_node_t *n) {
  if (t == NULL || n == NULL) {
    return;
  }

  if (t->root == n) {
    t->root = NULL;
    return;
  }

  as_rb_node_t *fa = n->parent;
  as_rb_node_t *gf = fa->parent;
  as_rb_node_t *s = fa->left == n ? fa->right : fa->left;
  if (fa->color == RED) {
    if (gf == NULL) {
      t->root = s;
    } else {
      if (gf->left == fa) {
        gf->left = s;
      } else {
        gf->right = s;
      }
    }
    if (s != NULL) {
      s->parent = gf;
    }

    rb_tree_isolated_node(fa);
  }
  n->parent = NULL;
}
