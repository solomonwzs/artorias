#include <unistd.h>
#include <string.h>
#include <sys/socket.h>
#include "utils.h"
#include "bytes.h"

#define MAX_BLOCK_SIZE 4096

#define to_block(_n_) (container_of(_n_, as_bytes_block_t, node))

static inline void
bytes_block_init(as_bytes_block_t *b, size_t size) {
  dlist_node_init(&b->node);
  b->size = size;
  b->used = 0;
}


void
bytes_init(as_bytes_t *bs, as_mem_pool_fixed_t *mp) {
  bs->size = 0;
  bs->used = 0;
  bs->cnt = 0;
  bs->mp = mp;
  dlist_init(&bs->dl);
}


size_t
bytes_read_from_fd(as_bytes_t *bs, int fd) {
  size_t nbyte;
  size_t rsize = 128;
  size_t n = 0;

  do {
    as_bytes_block_t *block = mpf_alloc(bs->mp, sizeof_bytes_block(rsize));
    bytes_block_init(block, rsize);
    void *ptr = block->d;

    nbyte = read(fd, ptr, rsize);
    if (nbyte < 0) {
      if (errno == EAGAIN) {
        mpf_recycle(block);
        break;
      } else {
        mpf_recycle(block);
        return -1;
      }
    } else {
      block->used = nbyte;

      bs->size += rsize;
      bs->used += nbyte;
      bs->cnt += 1;
      dlist_add_to_tail(&bs->dl, &block->node);

      n += nbyte;

      if (rsize < MAX_BLOCK_SIZE) {
        rsize <<= 1;
      }
    }
  } while (nbyte > 0);

  return n;
}


extern size_t
bytes_copy_to(as_bytes_t *bs, void *ptr, size_t offset, size_t n) {
  as_dlist_node_t *node = bs->dl.head;
  while (offset != 0) {
    as_bytes_block_t *block = to_block(node);
  }
  return 0;
}


void
bytes_destroy(as_bytes_t *bs) {
  bs->size = 0;
  bs->used = 0;
  bs->cnt = 0;

  dlist_pos_travel(bs->dl.head, mpf_recycle);
  dlist_init(&bs->dl);
}
