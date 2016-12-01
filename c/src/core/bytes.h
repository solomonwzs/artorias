#ifndef __BYTES__
#define __BYTES__

#include "mem_pool.h"

typedef struct as_bytes_block_s {
  struct as_bytes_block_s   *next;
  size_t                    size;
  size_t                    used;
  unsigned char             d[1];
} as_bytes_block_t;

typedef struct {
  size_t                size;
  size_t                used;
  int                   cnt;
  as_mem_pool_fixed_t   *mp;
  as_bytes_block_t      *head;
  as_bytes_block_t      *curr;
} as_bytes_t;

extern void
bytes_init(as_bytes_t *bs, as_mem_pool_fixed_t *mp);

extern int
bytes_append(as_bytes_t *bs, const void *src, size_t len);

extern ssize_t
bytes_write_to_fd(int fd, as_bytes_t *bs, size_t nbyte);

extern void
bytes_destroy(as_bytes_t *bs);

extern void
bytes_print(as_bytes_t *bs);

extern ssize_t
bytes_read_from_fd(as_bytes_t *bs, int fd);

extern void
bytes_reset_used(as_bytes_t *bs, size_t n);

#endif
