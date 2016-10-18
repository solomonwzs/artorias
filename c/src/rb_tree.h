#ifndef __RB_TREE__
#define __RB_TREE__

#include <stdlib.h>

typedef enum {RED, BLACK} rb_color_t;
typedef enum {EQ, GT, LT} rb_comp_t;

typedef struct as_rb_node_s {
  rb_color_t            color;
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

#endif
