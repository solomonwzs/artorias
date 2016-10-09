#ifndef __MEM_POOL__
#define __MEM_POOL__

#include <stdint.h>
#include <stdlib.h>
#include <sys/types.h>

typedef struct as_mem_pool_s as_mem_pool_t;
typedef struct as_mb_small_s as_mb_small_t;
typedef struct as_mb_large_s as_mb_large_t;

struct as_mem_pool_s {
  size_t            max;

  as_mb_small_t    *empty;
  as_mb_small_t    *partial;
  as_mb_small_t    *full;

  as_mb_large_t    *large;
};

struct as_mb_small_s {
  size_t          size;
  u_char          *last;
  unsigned        cnt;
  uint8_t         failed;
  as_mb_small_t   *next;
  u_char          d[1];
};

struct as_mb_large_s {
  as_mb_large_t   *next;
  u_char          d[1];
};

#endif
