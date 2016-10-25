#include "rb_tree.h"


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


static inline as_rb_node_t *
sibling(as_rb_node_t *n) {
  as_rb_node_t *fa;
  if (n == NULL || (fa = n->parent) == NULL) {
    return NULL;
  }
  return n == fa->left ? fa->right : fa->left;
}


static inline void
rotate_rigth(as_rb_tree_t *t, as_rb_node_t *q) {
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
delete_case(as_rb_tree_t *t, as_rb_node_t *n) {
  if (t == NULL && n == NULL) {
    return;
  }

  as_rb_node_t *s;
  while (1) {
    if (n->parent == NULL) {
      return;
    }

    s = sibling(n);
    if (s->color == RED) {
      n->parent->color = RED;
      s->color = BLACK;
      if (n == n->parent->left) {
        rotate_left(t, s);
      } else {
        rotate_rigth(t, s);
      }
      s = sibling(n);
    }

    if (s->color == BLACK &&
        n->parent->color == BLACK &&
        s->left->color == BLACK &&
        s->right->color == BLACK) {
      s->color = RED;
      n = n->parent;
      continue;
    }

    if (n->parent->color == RED &&
        s->color == BLACK &&
        s->left->color == BLACK &&
        s->right->color == BLACK) {
      s->color = RED;
      n->parent->color = BLACK;
      return;
    }

    if (s->color == BLACK) {
      if (n == n->parent->left &&
          s->right->color == BLACK &&
          s->left->color == RED) {
        s->color = RED;
        s->left->color = BLACK;
        rotate_rigth(t, s);
        s = sibling(n);
      } else if (n == n->parent->right &&
                 s->left->color == BLACK &&
                 s->right->color == RED) {
        s->color = RED;
        s->right->color = BLACK;
        rotate_left(t, s);
        s = sibling(n);
      }
    }

    s->color = n->parent->color;
    n->parent->color = BLACK;
    if (n == n->parent->left) {
      s->right->color = BLACK;
      rotate_left(t, n->parent);
    } else {
      s->left->color = BLACK;
      rotate_rigth(t, n->parent);
    }
    return;
  }
}


static void inline
swap_and_remove_ori_node(as_rb_tree_t *t, as_rb_node_t *ori, as_rb_node_t *n) {
  as_rb_node_t *fa;
  as_rb_node_t *rc;

  if ((fa = ori->parent) == NULL) {
    t->root = n;
  } else {
    if (ori == fa->left) {
      fa->left = n;
    } else {
      fa->right = n;
    }
  }

  rc = n->right;
  fa = n->parent;
  if (fa == ori) {
    fa = n;
  } else {
    if (rc != NULL) {
      rc->parent = fa;
    }
    fa->left = rc;
    n->right = ori->right;
    ori->right->parent = n;
  }

  n->parent = ori->parent;
  n->color = ori->color;
  n->left = ori->left;
  ori->left->parent = n;
}


static void inline
remove_node_has_one_child(as_rb_tree_t *t, as_rb_node_t *n) {
  as_rb_node_t *fa = n->parent;
  as_rb_node_t *child = n->left == NULL ? n->right : n->left;
  if (child != NULL) {
    child->parent = fa;
  }
  if (fa != NULL) {
    if (n == fa->left) {
      fa->left = child;
    } else {
      fa->right = child;
    }
  } else {
    t->root = child;
  }
}


void
rb_delete(as_rb_tree_t *t, as_rb_node_t *n) {
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

      gf = grandparent(n);
      gf->color = RED;
      n = gf;

      continue;
    }

    gf = grandparent(n);
    if (n == fa->right && fa == gf->left) {
      rotate_left(t, fa);
      n = n->left;

      fa = n->parent;
      gf = fa->parent;
    } else if (n == fa->left && fa == gf->right) {
      rotate_rigth(t, fa);
      n = n->right;

      fa = n->parent;
      gf = fa->parent;
    }

    fa->color = BLACK;
    gf->color = RED;
    if (n == fa->left) {
      rotate_rigth(t, gf);
    } else {
      rotate_left(t, gf);
    }
  }
  t->root->color = BLACK;
}


void
rb_tree_destroy(as_rb_tree_t *t, void *data,
                void (*node_free)(as_rb_node_t *, void *)) {
  if (t == NULL) {
    return;
  }

  as_rb_node_t *tn = t->root;
  as_rb_node_t *leaf = &t->leaf;
  while (tn != NULL) {
    if (tn->left != leaf) {
      tn = tn->left;
    } else if (tn->right != leaf) {
      tn = tn->right;
    } else {
      as_rb_node_t *fa = tn->parent;
      if (fa != NULL) {
        if (tn == fa->left) {
          fa->left = leaf;
        } else {
          fa->right = leaf;
        }
      }
      node_free(tn, data);
      tn = fa;
    }
  }
}
