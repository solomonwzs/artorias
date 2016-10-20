#ifndef __RB_TREE__
#define __RB_TREE__

#include <stdlib.h>
#include "utils.h"

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

#define sizeof_rb_node(_s_) (offsetof(as_rb_node_t, d) + (_s_))

typedef struct {
  as_rb_node_t  *root;
  as_rb_node_t  leaf;
  rb_comp_t     (*comp)(void *a, void *b);
} as_rb_tree_t;

extern void
rb_tree_init(as_rb_tree_t *t, rb_comp_t (*comp)(void *a, void *b));

extern void
rb_tree_insert(as_rb_tree_t *t, as_rb_node_t *n);

extern void
rb_tree_delete(as_rb_tree_t *t, as_rb_node_t *n);

extern void
rb_tree_destroy(as_rb_tree_t *t, void *data,
                void (*node_free)(as_rb_node_t *, void *));

#endif
