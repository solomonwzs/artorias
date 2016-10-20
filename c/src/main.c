#include "epoll_server.h"
#include "select_server.h"
#include "channel.h"
#include "mem_buddy.h"
#include "mem_slot.h"
#include "mem_pool.h"
#include "rb_tree.h"
#include <unistd.h>

#define PORT 5555


void
server_test() {
  int sock;
  sock = make_socket(PORT);
  set_non_block(sock);
  if (listen(sock, 500) < 0) {
    perror("listen");
    exit(EXIT_FAILURE);
  }
  // select_server(sock);
  // epoll_server(sock);
  master_workers_server(sock, 2);
}


void
mem_buddy_test() {
  as_mem_buddy_t *b = buddy_new(4);
  unsigned x = buddy_alloc(b, 4);
  buddy_alloc(b, 8);
  buddy_alloc(b, 2);
  buddy_free(b, x);
  buddy_print(b);
  buddy_destroy(b);
}


void
mem_slot_test() {
  as_mem_slot_t *s = slot_new(4);
  slot_alloc(s, 4);
  int a = slot_alloc(s, 5);
  slot_alloc(s, 3);
  slot_free(s, a, 5);
  slot_alloc(s, 6);

  slot_print(s);
  slot_destroy(s);
}


void
mem_pool_test() {
  size_t s[] = {8, 12, 16, 24, 32, 48, 64, 128, 256};
  as_mem_pool_fixed_t *p = mem_pool_fixed_new(s, sizeof(s) / sizeof(s[0]));

  void *a = mem_pool_fixed_alloc(p, 18);
  printf("%p\n", a);
  printf("%d\n", p->empty);
  mem_pool_fixed_recycle(a);
  printf("%d\n", p->empty);
  void *b = mem_pool_fixed_alloc(p, 18);
  printf("%p\n", b);
  mem_pool_fixed_recycle(b);

  mem_pool_fixed_destroy(p);
}

rb_comp_t
rb_node_comp(void *a, void *b) {
  int *i = (int *)a;
  int *j = (int *)b;

  if (*i == *j) {
    return EQ;
  } else if (*i > *j) {
    return GT;
  } else {
    return LT;
  }
}

void
rb_node_free(as_rb_node_t *n, void *data) {
  int *i = (int *)n->d;
  debug_log("%d\n", *i);
  mem_pool_fixed_recycle(n);
}

void
rb_tree_print(as_rb_node_t *n, as_rb_node_t *leaf) {
  printf("{");
  if (n->left != leaf) {
    rb_tree_print(n->left, leaf);
  }
  printf("[%d %d]", *((int *)n->d), n->color);
  if (n->right != leaf) {
    rb_tree_print(n->right, leaf);
  }
  printf("}");
}

void
rb_tree_test() {
  size_t s[] = {8, 12, 16, 24, 32, 48, 64, 128, 256};
  as_mem_pool_fixed_t *pool = mem_pool_fixed_new(s, sizeof(s) / sizeof(s[0]));

  int arr[] = {12, 34, 54, 13, 78, 92, 11, 4, 48};
  as_rb_tree_t *tree = mem_pool_fixed_alloc(pool, sizeof(as_rb_tree_t));
  rb_tree_init(tree, rb_node_comp);
  for (int i = 0; i < sizeof(arr) / sizeof(arr[0]); ++i) {
    as_rb_node_t *node = mem_pool_fixed_alloc(
        pool, sizeof_rb_node(sizeof(int)));
    *((int *)node->d) = arr[i];
    rb_tree_insert(tree, node);
    rb_tree_print(tree->root, &(tree->leaf)); printf("\n");
  }
  rb_tree_destroy(tree, NULL, rb_node_free);
  mem_pool_fixed_recycle(tree);
  mem_pool_fixed_destroy(pool);
}


int
main(int argc, char **argv) {
  rb_tree_test();
  return 0;
}
