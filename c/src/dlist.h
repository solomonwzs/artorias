#ifndef __AS_DLIST__
#define __AS_DLIST__

#include "utils.h"

typedef struct as_dlist_node_s {
  struct as_dlist_node_s  *next;
  struct as_dlist_node_s  *prev;
} as_dlist_node_t;

typedef struct {
  as_dlist_node_t *head;
  as_dlist_node_t *tail;
} as_dlist_t;

#define dlist_node_init(_n_) do {\
  (_n_)->next = NULL;\
  (_n_)->prev = NULL;\
} while (0)

#define dlist_init(_dl_) do {\
  (_dl_)->head = NULL;\
  (_dl_)->tail = NULL;\
} while (0)

extern void
dlist_add_to_head(as_dlist_t *dlist, as_dlist_node_t *node);

extern void
dlist_add_to_tail(as_dlist_t *dlist, as_dlist_node_t *node);

extern void
dlist_del(as_dlist_t *dlist, as_dlist_node_t *node);

#define dlist_pos_travel(_dln_, _func_, ...) do {\
  as_dlist_node_t *__n = (_dln_);\
  while (__n != NULL) {\
    as_dlist_node_t *__next = __n->next;\
    _func_(__n, ## __VA_ARGS__);\
    __n = __next;\
  }\
} while (0)

#endif
