#include "rb_tree.h"

#define at_left_side(_n_) ((_n_) == (_n_)->parent->left)
#define at_right_side(_n_) ((_n_) == (_n_)->parent->right)


void
rb_tree_init(as_rb_tree_t *t, rb_comp_t (*comp)(void *a, void *b)) {
  t->root = NULL;
  t->comp = comp;
  t->leaf.color = BLACK;
  t->leaf.left = t->leaf.right = NULL;
}


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

  if (q->parent == NULL) {
    t->root = p;
  } else {
    if (at_left_side(q)) {
      q->parent->left = p;
    } else {
      q->parent->right = p;
    }
  }
  p->right = q;
  p->parent = q->parent;
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

  if (p->parent == NULL) {
    t->root = q;
  } else {
    if (at_left_side(p)) {
      p->parent->left = q;
    } else {
      p->parent->right = q;
    }
  }
  q->left = p;
  q->parent = p->parent;
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
      if (at_left_side(n)) {
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
      if (at_left_side(n) &&
          s->right->color == BLACK &&
          s->left->color == RED) {
        s->color = RED;
        s->left->color = BLACK;
        rotate_rigth(t, s);
        s = sibling(n);
      } else if (at_right_side(n) &&
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
    if (at_left_side(n)) {
      s->right->color = BLACK;
      rotate_left(t, n->parent);
    } else {
      s->left->color = BLACK;
      rotate_rigth(t, n->parent);
    }
    return;
  }
}


void
rb_tree_delete(as_rb_tree_t *t, as_rb_node_t *n) {
  as_rb_node_t *leaf = &(t->leaf);
  if (n->parent == NULL && n->left == leaf && n->right == leaf) {
    t->root = NULL;
    return;
  }

  as_rb_node_t *child = n->left != leaf ? n->left : n->right;
  if (n->parent == NULL) {
    child->parent = NULL;
    child->color = BLACK;
    t->root = child;
    return;
  }

  if (at_left_side(n)) {
    n->parent->left = child;
  } else {
    n->parent->right = child;
  }
  child->parent = n->parent;

  if (n->color == BLACK) {
    if (child->color == RED) {
      child->color = BLACK;
    } else {
      delete_case(t, child);
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

    if (at_right_side(n) && at_left_side(n->parent)) {
      rotate_left(t, n->parent);
      n = n->left;
    } else if (at_left_side(n) && at_right_side(n->parent)) {
      rotate_rigth(t, n->parent);
      n = n->right;
    }

    n->parent->color = BLACK;
    grandparent(n)->color = RED;
    if (at_left_side(n)) {
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

  n->color = RED;
  as_rb_node_t *tn = t->root;
  as_rb_node_t *leaf = &(t->leaf);
  if (tn == NULL) {
    n->color = BLACK;
    n->parent = NULL;
    n->left = n->right = leaf;
    t->root = n;
    return;
  }

  while (1) {
    if (t->comp((void *)tn->d, (void *)n->d) != LT) {
      if (tn->left != leaf) {
        tn = tn->left;
      } else {
        n->left = n->right = leaf;
        n->parent = tn;
        tn->left = n;
        insert_case(t, n);
        return;
      }
    } else {
      if (tn->right != leaf) {
        tn = tn->right;
      } else {
        n->left = n->right = leaf;
        n->parent = tn;
        tn->right = n;
        insert_case(t, n);
        return;
      }
    }
  }
}


void
rb_tree_destroy(as_rb_tree_t *t, void *data,
                void (*node_free)(as_rb_node_t *, void *)) {
  if (t == NULL) {
    return;
  }

  as_rb_node_t *tn = t->root;
  as_rb_node_t *leaf = &(t->leaf);
  while (tn != NULL) {
    if (tn->left != leaf) {
      tn = tn->left;
    } else if (tn->right != leaf) {
      tn = tn->right;
    } else {
      as_rb_node_t *fa = tn->parent;
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
