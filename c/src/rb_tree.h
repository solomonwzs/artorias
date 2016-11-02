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
  (_node_)->parent = (_node_)->left = (_node_)->right = NULL;\
  (_node_)->color = RED;\
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

#define rb_tree_postorder_travel(_t_, _func_, ...) do {\
  as_rb_node_t *__tn = (_t_)->root;\
  while (__tn != NULL) {\
    if (__tn->left != NULL) {\
      __tn = __tn->left;\
    } else if (__tn->right != NULL) {\
      __tn = __tn->right;\
    } else {\
      as_rb_node_t *__fa = __tn->parent;\
      if (__fa != NULL) {\
        if (__tn == __fa->left) {\
          __fa->left = NULL;\
        } else {\
          __fa->right = NULL;\
        }\
      }\
      _func_(__tn, ## __VA_ARGS__);\
      __tn = __fa;\
    }\
  }\
} while (0)

extern void
rb_tree_insert_case(as_rb_tree_t *t, as_rb_node_t *n);

extern void
rb_tree_delete(as_rb_tree_t *t, as_rb_node_t *n);

extern void
rb_tree_insert_to_most_left(as_rb_node_t **root, as_rb_node_t *n);

extern void
rb_tree_insert_to_most_right(as_rb_node_t **root, as_rb_node_t *n);

extern void
rb_tree_test();

#endif
