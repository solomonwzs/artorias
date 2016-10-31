#ifndef __RB_TREE__
#define __RB_TREE__

#include <stdlib.h>
#include "utils.h"

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
} as_rb_tree_t;

#define rb_tree_insert(_tree_, _node_, _lt_) do {\
  as_rb_node_t **__ptr = &((_tree_)->root);\
  while (*__ptr != NULL) {\
    (_node_)->parent = *__ptr;\
    __ptr = \
    _lt_((_node_), *__ptr) ? &(*__ptr)->left : &(*__ptr)->right;\
  }\
  *__ptr = (_node_);\
} while(0)

#define rb_tree_find(_tree_, _key_, _eq_, _lt_) ({\
  as_rb_node_t *__ptr = (_tree_)->root;\
  while (__ptr != NULL) {\
    if (_eq_((_key_), __ptr)) {\
      break;\
    } else {\
      __ptr = _lt_((_key_), __ptr) ? __ptr->left : __ptr->right;\
    }\
  }\
  __ptr;\
})

#define rb_tree_node_init(_n_) do {\
  (_n_)->parent = (_n_)->left = (_n_)->right = NULL;\
  (_n_)->color = RED;\
} while (0)

extern void
rb_tree_insert_case(as_rb_tree_t *t, as_rb_node_t *n);

extern void
rb_tree_delete(as_rb_tree_t *t, as_rb_node_t *n);

extern void
rb_tree_postorder_travel(as_rb_tree_t *t, void *data,
                         void (*func)(as_rb_node_t *, void *));

extern void
rb_tree_test();

#endif
