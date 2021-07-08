#include "bytes.h"

#include <string.h>
#include <sys/socket.h>
#include <unistd.h>

#include "utils.h"

#define MAX_BLOCK_SIZE 1024

#define to_block(_n_) (container_of(_n_, as_bytes_block_t, node))

static inline void bytes_block_init(as_bytes_block_t *b, size_t size) {
  dlist_node_init(&b->node);
  b->size = size;
  b->used = 0;
}

void bytes_init(as_bytes_t *bs, as_mem_pool_fixed_t *mp) {
  bs->size = 0;
  bs->used = 0;
  bs->cnt = 0;
  bs->mp = mp;
  dlist_init(&bs->dl);
}

size_t bytes_read_from_fd(as_bytes_t *bs, int fd) {
  size_t nbyte;
  size_t rsize = 64;
  size_t n = 0;

  do {
    as_bytes_block_t *block = memp_alloc(bs->mp, sizeof_bytes_block(rsize));
    bytes_block_init(block, rsize);
    void *ptr = block->d;

    nbyte = read(fd, ptr, rsize);
    if (nbyte < 0) {
      if (errno == EAGAIN) {
        memp_recycle(block);
        break;
      } else {
        memp_recycle(block);
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

static inline int bytes_offset(as_bytes_t *bs, size_t offset, int *out_n,
                               size_t *out_m) {
  if (offset >= bs->used) {
    return 1;
  }

  *out_n = 0;
  *out_m = 0;
  as_dlist_node_t *node = bs->dl.head;

  while (node != NULL) {
    as_bytes_block_t *b = to_block(node);

    if (offset < b->used) {
      *out_m = offset;
      return 0;
    } else {
      offset -= b->used;
      *out_n += 1;
      node = node->next;
    }
  }

  return 1;
}

size_t bytes_copy_to(as_bytes_t *bs, void *ptr, size_t offset, size_t n) {
  as_dlist_node_t *node = bs->dl.head;
  while (offset != 0) {
    as_bytes_block_t *block = to_block(node);
  }
  return 0;
}

size_t bytes_length(as_bytes_t *bs) {
  return bs->used;
}

void bytes_destroy(as_bytes_t *bs) {
  bs->size = 0;
  bs->used = 0;
  bs->cnt = 0;

  dlist_pos_travel(bs->dl.head, memp_recycle);
  dlist_init(&bs->dl);
}
