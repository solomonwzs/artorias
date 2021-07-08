#ifndef __AS_THREAD__
#define __AS_THREAD__

#include <stdint.h>
#include <time.h>

#include "dlist.h"
#include "lua_adapter.h"
#include "rb_tree.h"
#include "utils.h"

#define AS_TSTATUS_READY 0x00
#define AS_TSTATUS_RUN   0x01
#define AS_TSTATUS_SLEEP 0x02
#define AS_TSTATUS_STOP  0x03

#define AS_RSTATUS_IDLE 0x00
#define AS_RSTATUS_EV   0x01

#define AS_TMODE_SIMPLE     0x00
#define AS_TMODE_LOOP_SOCKS 0x01

struct as_thread_s;
struct as_thread_res_s;

typedef int (*as_thread_res_free_f)(struct as_thread_res_s *res, void *f_ptr);
typedef int (*as_thread_res_fd_f)(struct as_thread_res_s *res);

typedef struct as_thread_res_s {
  as_thread_res_free_f freef;
  as_thread_res_fd_f fdf;
  uint8_t status;
  as_dlist_node_t node;
  struct as_thread_s *th;
  uint8_t d[];
} as_thread_res_t;

typedef struct as_thread_s {
  as_tid_t tid;
  lua_State *T;
  time_t et;
  as_rb_node_t p_idx;
  as_rb_tree_t *pool;
  uint8_t status;
  uint8_t mode;
  as_thread_res_t *mfd_res;
  as_dlist_t resl;
} as_thread_t;

typedef struct {
  int n;
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

#define asthread_th_is_ready(_th_) ((_th_)->status == AS_TSTATUS_READY)

#define asthread_th_is_stop(_th_) ((_th_)->status == AS_TSTATUS_STOP)

#define asthread_th_to_stop(_th_, _arr_)   \
  do {                                     \
    (_th_)->status = AS_TSTATUS_STOP;      \
    (_th_)->pool = NULL;                   \
    asthread_th_add_to_array(_th_, _arr_); \
  } while (0)

#define asthread_th_to_sleep(_th_, _et_, _pool_) \
  do {                                           \
    (_th_)->et = (_et_);                         \
    (_th_)->status = AS_TSTATUS_SLEEP;           \
    asthread_th_add_to_pool(_th_, _pool_);       \
  } while (0)

#define asthread_th_to_iowait(_th_, _et_, _pool_) \
  do {                                            \
    (_th_)->et = (_et_);                          \
    (_th_)->status = AS_TSTATUS_RUN;              \
    asthread_th_add_to_pool(_th_, _pool_);        \
  } while (0)

#define asthread_th_is_simple_mode(_th_) ((_th_)->mode == AS_TMODE_SIMPLE)

extern int asthread_th_init(as_thread_t *th, lua_State *L);

extern int asthread_th_free(as_thread_t *th, void *f_ptr);

extern int asthread_th_add_to_pool(as_thread_t *th, as_rb_tree_t *pool);

extern int asthread_th_del_from_pool(as_thread_t *th);

extern void asthread_th_add_to_array(as_thread_t *th, as_thread_array_t *array);

extern int asthread_res_init(as_thread_res_t *res, as_thread_res_free_f freef,
                             as_thread_res_fd_f fdf);

extern int asthread_res_add_to_th(as_thread_res_t *res, as_thread_t *th);

extern int asthread_res_del_from_th(as_thread_res_t *res, as_thread_t *th);

extern int asthread_res_ev_init(as_thread_res_t *res, int epfd);

extern int asthread_res_ev_add(as_thread_res_t *res, int epfd, uint32_t events);

extern int asthread_res_ev_del(as_thread_res_t *res, int epfd);

extern as_rb_node_t *asthread_remove_timeout_threads(as_rb_tree_t *pool);

extern void asthread_remove_res_from_epfd(as_thread_t *th, int epfd);

extern void asthread_print_res(as_thread_t *th);

#endif
