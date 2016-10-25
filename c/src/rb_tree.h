#ifndef __RB_TREE__
#define __RB_TREE__

#include <stdlib.h>
#include "utils.h"

typedef enum {EQ, GT, LT} rb_comp_t;

#define RED     0x00
#define BLACK   0x01

typedef struct as_rb_node_s {
  struct as_rb_node_s   *parent;
  struct as_rb_node_s   *left;
  struct as_rb_node_s   *right;
  char                  color;
} as_rb_node_t;

typedef struct {
  as_rb_node_t  *root;
  as_rb_node_t  leaf;
} as_rb_tree_t;

extern void
rb_tree_insert_case(as_rb_tree_t *t, as_rb_node_t *n);

extern void
rb_tree_destroy(as_rb_tree_t *t, void *data,
                void (*node_free)(as_rb_node_t *, void *));

extern void
rb_tree_test();

#endif
