#ifndef __MEM_POOL__
#define __MEM_POOL__

#include <stdint.h>
#include <stdlib.h>
#include "utils.h"

typedef union {
  struct as_mem_pool_fixed_field_s  *field;
  struct as_mem_data_fixed_s        *next;
} as_mem_data_fixed_p_t;

typedef struct as_mem_data_fixed_s {
  as_mem_data_fixed_p_t   p;
  uint8_t                 d[1];
} as_mem_data_fixed_t;

typedef struct as_mem_pool_fixed_field_s {
  size_t                      size;
  struct as_mem_pool_fixed_s  *pool;
  as_mem_data_fixed_t         *header;
} as_mem_pool_fixed_field_t;

typedef struct as_mem_pool_fixed_s {
  int                         empty;
  int                         used;
  uint8_t                     n;
  as_mem_pool_fixed_field_t   f[1];
} as_mem_pool_fixed_t;

#define DEFAULT_FIXED_SIZE \
{8, 12, 16, 24, 32, 48, 64, 128, 256, 384, 512, 768, 1024}

#define mpf_data_size(_d_) \
    ((container_of((_d_), as_mem_data_fixed_t, d))->p.field->size)

extern as_mem_pool_fixed_t *
mpf_new(size_t fsize[], unsigned n);

extern void
mpf_destroy(as_mem_pool_fixed_t *p);

extern void *
mpf_alloc(as_mem_pool_fixed_t *p, size_t size);

extern void
mpf_recycle(void *dd);

#endif
