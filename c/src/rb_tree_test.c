#include <stdio.h>
#include "mem_pool.h"
#include "rb_tree.h"
#include "utils.h"

#define to_node(_p_) (container_of(_p_, node, r))
#define node_lt(_a_, _b_) \
    (container_of(_a_, node, r)->i < container_of(_b_, node, r)->i)
#define node_i_lt(_i_, _a_) \
    ((_i_) < container_of(_a_, node, r)->i)
#define node_i_eq(_i_, _a_) \
    ((_i_) == container_of(_a_, node, r)->i)


typedef struct {
  as_rb_node_t  r;
  int           i;
} node;


static void
rb_tree_print(node *n) {
  printf("{");
  if (n != NULL) {
    if (n->r.left != NULL) {
      rb_tree_print(to_node(n->r.left));
    }

    if (n->r.color == RED) {
      printf("\033[1;31m %d \033[0m", n->i);
    } else {
      printf(" %d ", n->i);
    }
    printf("\033[0;37m%lx %lx \033[0m",
           (unsigned long)(&n->r) & 0xfff,
           (unsigned long)(n->r.parent) & 0xfff);

    if (n->r.right != NULL) {
      rb_tree_print(to_node(n->r.right));
    }
  }
  printf("}");
}


void
rb_tree_test() {
  size_t s[] = {8, 12, 16, 24, 32, 48, 64, 128, 256, 384, 512, 768, 1024};
  as_mem_pool_fixed_t *pool = mem_pool_fixed_new(s, sizeof(s) / sizeof(s[0]));

  // int arr[] = {34, 5, 12, 15, 71, 45, 2, 33, 3, 83, 61, 43, 11};
  int arr[] = {1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1,
    2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2};
  int n = sizeof(arr) / sizeof(arr[0]);

  node *ns = mem_pool_fixed_alloc(pool, sizeof(node) * n);
  as_rb_tree_t *tree = mem_pool_fixed_alloc(pool, sizeof(as_rb_tree_t));
  tree->root = NULL;

  for (int i = 0; i < n; ++i) {
    node *m = &ns[i];
    m->i = arr[i];
    rb_tree_node_init(&m->r);
    rb_tree_insert(tree, &m->r, node_lt);
    rb_tree_insert_case(tree, &m->r);
    rb_tree_print(to_node(tree->root));
    printf("\n");
  }

  for (int i = 0; i < n; ++i) {
    // node *m = &ns[i];
    as_rb_node_t *n = rb_tree_find(tree, arr[i], node_i_eq, node_i_lt);
    printf("%d, %p\n", arr[i], n);
    node *m = container_of(n, node, r);

    rb_tree_delete(tree, &m->r);
    rb_tree_print(to_node(tree->root));
    printf("\n");
  }

  mem_pool_fixed_recycle(tree);
  mem_pool_fixed_recycle(ns);
  mem_pool_fixed_destroy(pool);
}
