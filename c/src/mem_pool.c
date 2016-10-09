#include "mem_pool.h"

as_mem_pool_t *
mem_pool_new(size_t size) {
  as_mem_pool_t *p = (as_mem_pool_t *)malloc(
      sizeof(as_mem_pool_t) - sizeof(u_char) + size);
  if (p == NULL) {
    return NULL;
  }

  p->max = size;
  return NULL;
}
