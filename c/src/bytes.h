#ifndef __BYTES__
#define __BYTES__

#include <string.h>
#include "utils.h"
#include "mem_pool.h"

typedef struct as_bytes_block_s {
  struct as_bytes_block_s   *next;
  size_t                    size;
  size_t                    used;
  char                      d[1];
} as_bytes_block_t;

typedef struct {
  size_t            size;
  size_t            used;
  int               cnt;
  as_bytes_block_t  *head;
  as_bytes_block_t  *tail;
} as_bytes_t;

#define NULL_AS_BYTES {0, 0, NULL, NULL}

#endif
