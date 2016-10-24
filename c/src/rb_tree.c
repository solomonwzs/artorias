#include "rb_tree.h"

#define at_left_side(n) ((n) == rb_node_parent(n)->left)
#define at_right_side(n) ((n) == rb_node_parent(n)->right)


static inline as_rb_node_t *
grandparent(as_rb_node_t *n) {
  if (n == NULL || rb_node_parent(n) == NULL) {
    return NULL;
  }
  return rb_node_parent(rb_node_parent(n));
}


static inline as_rb_node_t *
uncle(as_rb_node_t *n) {
  as_rb_node_t *g = grandparent(n);
  if (g == NULL) {
    return NULL;
  }
  return rb_node_parent(n) == g->right ? g->left : g->right;
}


static inline as_rb_node_t *
sibling(as_rb_node_t *n) {
  as_rb_node_t *fa;
  if (n == NULL || (fa = rb_node_parent(n)) == NULL) {
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
    rb_node_set_parent(y, q);
  }
  q->left = y;

  as_rb_node_t *fa = rb_node_parent(q);
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
  rb_node_set_parent(p, fa);
  rb_node_set_parent(q, p);
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
    rb_node_set_parent(y, p);
  }
  p->right = y;

  as_rb_node_t *fa = rb_node_parent(p);
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
  rb_node_set_parent(q, fa);
  rb_node_set_parent(p, q);
}


static inline as_rb_node_t *
get_most_left_node(as_rb_tree_t *t, as_rb_node_t *n) {
  if (t == NULL || n == NULL) {
    return NULL;
  }

  as_rb_node_t *leaf = &t->leaf;
  while (n->left != leaf) {
    n = n->left;
  }
  return n;
}


static inline as_rb_node_t *
get_most_rigth_node(as_rb_tree_t *t, as_rb_node_t *n) {
  if (t == NULL || n == NULL) {
    return NULL;
  }

  as_rb_node_t *leaf = &t->leaf;
  while (n->right != leaf) {
    n = n->right;
  }
  return n;
}


// static void
// delete_case(as_rb_tree_t *t, as_rb_node_t *n) {
//   if (t == NULL && n == NULL) {
//     return;
//   }
// 
//   as_rb_node_t *s;
//   while (1) {
//     if (n->parent == NULL) {
//       return;
//     }
// 
//     s = sibling(n);
//     if (s->color == RED) {
//       n->parent->color = RED;
//       s->color = BLACK;
//       if (at_left_side(n)) {
//         rotate_left(t, s);
//       } else {
//         rotate_rigth(t, s);
//       }
//       s = sibling(n);
//     }
// 
//     if (s->color == BLACK &&
//         n->parent->color == BLACK &&
//         s->left->color == BLACK &&
//         s->right->color == BLACK) {
//       s->color = RED;
//       n = n->parent;
//       continue;
//     }
// 
//     if (n->parent->color == RED &&
//         s->color == BLACK &&
//         s->left->color == BLACK &&
//         s->right->color == BLACK) {
//       s->color = RED;
//       n->parent->color = BLACK;
//       return;
//     }
// 
//     if (s->color == BLACK) {
//       if (at_left_side(n) &&
//           s->right->color == BLACK &&
//           s->left->color == RED) {
//         s->color = RED;
//         s->left->color = BLACK;
//         rotate_rigth(t, s);
//         s = sibling(n);
//       } else if (at_right_side(n) &&
//                  s->left->color == BLACK &&
//                  s->right->color == RED) {
//         s->color = RED;
//         s->right->color = BLACK;
//         rotate_left(t, s);
//         s = sibling(n);
//       }
//     }
// 
//     s->color = n->parent->color;
//     n->parent->color = BLACK;
//     if (at_left_side(n)) {
//       s->right->color = BLACK;
//       rotate_left(t, n->parent);
//     } else {
//       s->left->color = BLACK;
//       rotate_rigth(t, n->parent);
//     }
//     return;
//   }
// }


static void inline
swap_node(as_rb_tree_t *t, as_rb_node_t *ori, as_rb_node_t *n) {
  as_rb_node_t *fa;
  as_rb_node_t *rc;

  if ((fa = rb_node_parent(ori)) == NULL) {
    t->root = n;
  } else {
    if (ori == fa->left) {
      fa->left = n;
    } else {
      fa->right = n;
    }
  }

  rc = n->right;
  fa = rb_node_parent(n);
  if (fa == ori) {
    fa = n;
  } else {
    if (rc != NULL) {
      rb_node_set_parent(rc, fa);
    }
    fa->left = rc;
    n->right = ori->right;
    rb_node_set_parent(ori->right, n);
  }

  n->__parent_color = ori->__parent_color;
  n->left = ori->left;
  rb_node_set_parent(ori->left, n);
}


void
rb_delete(as_rb_tree_t *t, as_rb_node_t *n) {
  if (t == NULL && n == NULL) {
    return;
  }

  as_rb_node_t *child;
  if (n->left == NULL) {
    child = n->right;
  } else if (n->right == NULL) {
    child = n->left;
  } else {
    as_rb_node_t *ori = n;
    as_rb_node_t *left;
    as_rb_node_t *fa;
    char color;

    n = n->right;
    while ((left = n->left) != NULL) {
      n = n->left;
    }
    swap_node(t, ori, n);
  }
}


void
rb_insert_case(as_rb_tree_t *t, as_rb_node_t *n) {
  if (t == NULL || n == NULL) {
    return;
  }

  as_rb_node_t *fa;
  as_rb_node_t *gf;
  as_rb_node_t *uc;
  while ((fa = rb_node_parent(n)) && rb_node_color(fa) == RED) {
    if ((uc = uncle(n)) && rb_node_color(uc) == RED) {
      rb_node_set_black(fa);
      rb_node_set_black(uc);

      gf = grandparent(n);
      rb_node_set_red(gf);
      n = gf;

      continue;
    }

    gf = grandparent(n);
    if (n == fa->right && fa == gf->left) {
      rotate_left(t, fa);
      n = n->left;

      fa = rb_node_parent(n);
      gf = rb_node_parent(fa);
    } else if (n == fa->left && fa == gf->right) {
      rotate_rigth(t, fa);
      n = n->right;

      fa = rb_node_parent(n);
      gf = rb_node_parent(fa);
    }

    rb_node_set_black(fa);
    rb_node_set_red(gf);
    if (n == fa->left) {
      rotate_rigth(t, gf);
    } else {
      rotate_left(t, gf);
    }
  }
  rb_node_set_black(t->root);
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
      as_rb_node_t *fa = rb_node_parent(tn);
      if (fa != NULL) {
        if (at_left_side(tn)) {
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
