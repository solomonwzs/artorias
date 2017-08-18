#ifndef __BYTES__
#define __BYTES__

#include <stdint.h>
#include "dlist.h"
#include "mem_pool.h"

typedef struct as_bytes_block_s {
  as_dlist_node_t node;
  size_t          size;
  size_t          used;
  uint8_t         d[];
} as_bytes_block_t;

typedef struct {
  size_t                size;
  size_t                used;
  int                   cnt;
  as_mem_pool_fixed_t   *mp;
  as_dlist_t            dl;
} as_bytes_t;

#define sizeof_bytes_block(_size_) \
    (offsetof(as_bytes_block_t, d) + _size_)

extern void
bytes_init(as_bytes_t *bs, as_mem_pool_fixed_t *mp);

extern size_t
bytes_read_from_fd(as_bytes_t *bs, int fd);

extern size_t
bytes_copy_to(as_bytes_t *bs, void *ptr, size_t offset, size_t n);

extern void
bytes_destroy(as_bytes_t *bs);

#endif
