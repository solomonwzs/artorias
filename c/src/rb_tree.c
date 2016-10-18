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
  if (n == NULL || n->parent == NULL) {
    return NULL;
  }
  return n == n->parent->left ? n->parent->right : n->parent->left;
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
  if (p == NULL) {
    return;
  }

  as_rb_node_t *y = p->right;
  if (y != NULL) {
    y->parent = q;
  }
  q->left = y;

  as_rb_node_t *fa = q->parent;
  if (fa == NULL) {
    t->root = p;
  } else {
    if (fa->left == q) {
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

  as_rb_node_t *fa = q->parent;
  if (fa == NULL) {
    t->root = p;
  } else {
    if (fa->left == p) {
      fa->left = q;
    } else {
      fa->right = q;
    }
  }
  q->left = p;
  q->parent = fa;
  p->parent = q;
}


static inline as_rb_node_t *
get_smallest_node(as_rb_tree_t *t) {
  if (t == NULL || t->root == NULL) {
    return NULL;
  }

  as_rb_node_t *n = t->root;
  while (n->left != NULL) {
    n = n->left;
  }
  return n;
}


static void
delete_case(as_rb_tree_t *t, as_rb_node_t *n) {
  while (1) {
    if (n->parent == NULL) {
      return;
    }

    as_rb_node_t *s = sibling(n);
    if (s->color == RED) {
      n->parent->color = RED;
      s->color = BLACK;
      if (n == n->parent->left) {
        rotate_left(t, s);
      } else {
        rotate_rigth(t, s);
      }
    }
  }
}


static void
insert_case(as_rb_tree_t *t, as_rb_node_t *n) {
  if (t == NULL || n == NULL) {
    return;
  }

  while (1) {
    if (n->parent == NULL) {
      t->root = n;
      n->color = BLACK;
      return;
    }

    if (n->parent->color == BLACK) {
      return;
    }

    if (uncle(n) != NULL && uncle(n)->color == RED) {
      n->parent->color = BLACK;
      uncle(n)->color = BLACK;
      grandparent(n)->color = RED;
      n = grandparent(n);
      continue;
    }

    if (n == n->parent->right && n->parent == grandparent(n)->left) {
      rotate_left(t, n->parent);
      n = n->left;
    } else if (n == n->parent->left && grandparent(n)->right) {
      rotate_rigth(t, n->parent);
      n = n->right;
    }

    n->parent->color = BLACK;
    grandparent(n)->color = RED;
    if (n == n->parent->left && n->parent == grandparent(n)->left) {
      rotate_rigth(t, grandparent(n));
    } else {
      rotate_left(t, grandparent(n));
    }
    return;
  }
}

void
rb_tree_insert(as_rb_tree_t *t, as_rb_node_t *n) {
  if (t == NULL || n == NULL) {
    return;
  }

  as_rb_node_t *tn = t->root;
  if (tn == NULL) {
    n->parent = n->left = n->right = NULL;
    t->root = n;
    return;
  }

  while (1) {
    if (t->comp((void *)tn->d, (void *)n->d) != LT) {
      if (tn->left != NULL) {
        tn = tn->left;
      } else {
        n->left = n->right = NULL;
        n->parent = tn;
        tn->left = n;
        insert_case(t, n);
        return;
      }
    } else {
      if (tn->right != NULL) {
        tn = tn->right;
      } else {
        n->left = n->right = NULL;
        n->parent = tn;
        tn->right = n;
        insert_case(t, n);
        return;
      }
    }
  }
}
