#ifndef __RB_TREE__
#define __RB_TREE__

#include <stdlib.h>

typedef enum {EQ, GT, LT} rb_comp_t;

#define BLACK   0x00
#define RED     0x01

typedef struct as_rb_node_s {
  char                  color;
  struct as_rb_node_s   *left;
  struct as_rb_node_s   *right;
  struct as_rb_node_s   *parent;
  char                  d[1];
} as_rb_node_t;

typedef struct {
  as_rb_node_t  *root;
  rb_comp_t     (*comp)(void *a, void *b);
} as_rb_tree_t;

extern void
rb_tree_insert(as_rb_tree_t *t, as_rb_node_t *n);

extern void
rb_tree_delete(as_rb_tree_t *t, as_rb_node_t *n);

#endif
