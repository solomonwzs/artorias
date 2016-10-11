#ifndef __MEM_POOL__
#define __MEM_POOL__

#include <stdint.h>
#include <stdlib.h>
#include <sys/types.h>

typedef struct as_mem_data_fix_s {
  struct as_mem_data_fix_s  *next;
  int                       idx;
  uint8_t                   d[1];
} as_mem_data_fix_t;

typedef struct {
  size_t              size;
  as_mem_data_fix_t   *header;
} as_mem_pool_fix_field_t;

typedef struct {
  unsigned                  n;
  unsigned                  used;
  unsigned                  empty;
  as_mem_pool_fix_field_t   f[1];
} as_mem_pool_fix_t;

#endif
