#ifndef __MEM_POOL__
#define __MEM_POOL__

#include <stdint.h>
#include <stdlib.h>
#include <sys/types.h>

#ifndef offsetof
#   ifdef __GNUC__
#     define offsetof(st, m) __builtin_offsetof(st, m)
#   else
#     define offsetof(st, m) ((size_t)&(((st *)0)->m))
#   endif
#endif

#define to_data_fixed(_p_) \
    ((as_mem_data_fixed_t *)((uint8_t *)(_p_) - \
                             offsetof(as_mem_data_fixed_t, d)))

typedef struct as_mem_data_fixed_s {
  struct as_mem_data_fixed_s  *next;
  size_t                      size;
  uint8_t                     d[1];
} as_mem_data_fixed_t;

typedef struct {
  size_t                size;
  as_mem_data_fixed_t   *header;
} as_mem_pool_fixed_field_t;

typedef struct {
  unsigned                    n;
  int                         used;
  int                         empty;
  as_mem_pool_fixed_field_t   f[1];
} as_mem_pool_fixed_t;

extern as_mem_pool_fixed_t *
mem_pool_fixed_new(size_t fsize[], unsigned n);

extern void
mem_pool_fixed_destroy(as_mem_pool_fixed_t *p);

extern void *
mem_pool_fixed_alloc(as_mem_pool_fixed_t *p, size_t size);

extern void
mem_pool_fixed_recycle(as_mem_pool_fixed_t *p, void *dd);

#endif
