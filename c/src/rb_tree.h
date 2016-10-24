#ifndef __RB_TREE__
#define __RB_TREE__

#include <stdlib.h>
#include "utils.h"

typedef enum {EQ, GT, LT} rb_comp_t;

#define RED     0x00
#define BLACK   0x01

typedef struct as_rb_node_s {
  unsigned long         __parent_color;
  struct as_rb_node_s   *left;
  struct as_rb_node_s   *right;
} __attribute__((aligned(sizeof(long)))) as_rb_node_t;

#define rb_node_parent(n) ((as_rb_node_t *)((n)->__parent_color & ~3))
#define rb_node_color(n) ((n)->__parent_color & 1)

#define rb_node_set_parent(n, p) do {\
  (n)->__parent_color = ((unsigned long)(p) & ~3) | (rb_node_color(n));\
} while(0)

#define rb_node_set_red(n) do {\
  (n)->__parent_color &= ~1;\
} while(0)

#define rb_node_set_black(n) do {\
  (n)->__parent_color |= 1;\
} while(0)

#define rb_node_eq(a, b) (((unsigned long)(a) & ~3) == \
                          ((unsigned long)(b) & ~3))

typedef struct {
  as_rb_node_t  *root;
  as_rb_node_t  leaf;
} as_rb_tree_t;

extern void
rb_tree_delete(as_rb_tree_t *t, as_rb_node_t *n);

extern void
rb_tree_destroy(as_rb_tree_t *t, void *data,
                void (*node_free)(as_rb_node_t *, void *));

#endif
