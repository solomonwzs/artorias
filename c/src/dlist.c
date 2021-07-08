#include "dlist.h"

#define dlist_is_empty(_dl_) ((_dl_)->head == NULL)

void dlist_add_to_head(as_dlist_t *dlist, as_dlist_node_t *node) {
  node->prev = NULL;
  node->next = dlist->head;

  if (!dlist_is_empty(dlist)) {
    dlist->head->prev = node;
  } else {
    dlist->tail = node;
  }
  dlist->head = node;
}

void dlist_add_to_tail(as_dlist_t *dlist, as_dlist_node_t *node) {
  node->next = NULL;
  node->prev = dlist->tail;

  if (!dlist_is_empty(dlist)) {
    dlist->tail->next = node;
  } else {
    dlist->head = node;
  }
  dlist->tail = node;
}

void dlist_del(as_dlist_t *dlist, as_dlist_node_t *node) {
  if (dlist->head == node) {
    dlist->head = node->next;
  }
  if (dlist->tail == node) {
    dlist->tail = node->prev;
  }

  as_dlist_node_t *prev = node->prev;
  as_dlist_node_t *next = node->next;

  if (prev != NULL) {
    prev->next = next;
  }
  if (next != NULL) {
    next->prev = prev;
  }
}
