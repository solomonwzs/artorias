#ifndef __MEM_POOL__
#define __MEM_POOL__

#include <stdint.h>
#include <stdlib.h>
#include <sys/types.h>

typedef struct as_mem_data_fix_s {
  struct as_mem_data_fix_s  *next;
  size_t                    size;
  uint8_t                   d[1];
} as_mem_data_fix_t;

typedef struct {
  size_t              size;
  as_mem_data_fix_t   *header;
} as_mem_pool_fix_field_t;

typedef struct {
  unsigned                  n;
  int                       used;
  int                       empty;
  as_mem_pool_fix_field_t   f[1];
} as_mem_pool_fix_t;

extern as_mem_pool_fix_t *
mem_pool_fix_new(size_t fsize[], unsigned n);

extern void
mem_pool_fix_destory(as_mem_pool_fix_t *p);

extern as_mem_data_fix_t *
mem_pool_fix_alloc(as_mem_pool_fix_t *p, size_t size);

extern void
mem_pool_fix_recycle(as_mem_pool_fix_t *p, as_mem_data_fix_t *d);

#endif
