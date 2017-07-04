#ifndef __AS_THREAD__
#define __AS_THREAD__

#include <stdint.h>
#include <time.h>
#include "lua_adapter.h"
#include "rb_tree.h"
#include "utils.h"

#define AS_TSTATUS_READY  0x00
#define AS_TSTATUS_RUN    0x01
#define AS_TSTATUS_SLEEP  0x02
#define AS_TSTATUS_STOP   0x03

#define AS_RSTATUS_IDLE   0x00
#define AS_RSTATUS_EPFD   0x01

struct as_thread_s;
struct as_thread_res_s;

typedef int (*as_thread_res_free_f)(struct as_thread_res_s *res, void *f_ptr);
typedef int (*as_thread_res_fd_f)(struct as_thread_res_s *res);

typedef struct as_dlist_node_s {
  struct as_dlist_node_s  *next;
  struct as_dlist_node_s  *prev;
} as_dlist_node_t;

typedef struct as_thread_res_s {
  as_thread_res_free_f  freef;
  as_thread_res_fd_f    fdf;
  int                   status;
  as_dlist_node_t       node;
  struct as_thread_s    *th;
  uint8_t               d[];
} as_thread_res_t;

typedef struct as_thread_s {
  as_tid_t          tid;
  lua_State         *T;
  time_t            ct;
  time_t            ut;
  time_t            et;
  as_rb_node_t      p_idx;
  as_rb_tree_t      *pool;
  int               status;
  as_thread_res_t   *mfd_res;
  as_dlist_node_t   *res_head;
} as_thread_t;

typedef struct {
  int         n;
  as_thread_t *ths[];
} as_thread_array_t;

#define sizeof_thread_res(_type_) \
    (offsetof(as_thread_res_t, d) + sizeof(_type_))

#define sizeof_thread_array(_n_) \
    (offsetof(as_thread_res_t, d) + sizeof(as_thread_t *) * (_n_))

#define rb_node_to_thread(_n_) \
    ((as_thread_t *)(container_of(_n_, as_thread_t, p_idx)))

#define dl_node_to_res(_n_) \
    ((as_thread_res_t *)(container_of(_n_, as_thread_res_t, node)))

#define resd_to_res(_n_) \
    ((as_thread_res_t *)(container_of(_n_, as_thread_res_t, d)))

extern int
asthread_init(as_thread_t *th, lua_State *L);

extern int
asthread_free(as_thread_t *th, void *f_ptr);

extern int
asthread_pool_insert(as_thread_t *th);

extern int
asthread_pool_delete(as_thread_t *th);

extern void
asthread_array_add(as_thread_array_t *array, as_thread_t *th);

extern int
asthread_res_add(as_thread_t *th, as_thread_res_t *res);

extern int
asthread_res_del(as_thread_t *th, as_thread_res_t *res);

extern as_rb_node_t*
asthread_remove_timeout_threads(as_rb_tree_t *pool);

#endif
