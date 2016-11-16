#ifndef __BYTES__
#define __BYTES__

#include "mem_pool.h"

typedef struct as_bytes_block_s {
  struct as_bytes_block_s   *next;
  size_t                    size;
  size_t                    used;
  char                      d[1];
} as_bytes_block_t;

typedef struct {
  size_t                size;
  size_t                used;
  int                   cnt;
  as_mem_pool_fixed_t   *mp;
  as_bytes_block_t      *head;
  as_bytes_block_t      *tail;
} as_bytes_t;

extern void
bytes_init(as_bytes_t *buf, as_mem_pool_fixed_t *mp);

extern void
bytes_append(as_bytes_t *buf, void *b, size_t len);

extern ssize_t
bytes_write_to_fd(int fd, as_bytes_t *buf, size_t nbyte);

extern void
bytes_destroy(as_bytes_t *buf);

extern void
bytes_print(as_bytes_t *buf);

#endif
