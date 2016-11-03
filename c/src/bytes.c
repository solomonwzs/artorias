#include "bytes.h"

#define MAX_DOUBLE_SIZE 512

#define available_size(_n_) ((_n_)->size - (_n_)->used)
#define bytes_block_a_ptr(_n_) ((void *)(&(_n_)->d[(_n_)->used]))
#define new_block_size(_s_) \
    (offsetof(as_bytes_block_t, d) + \
     ((_s_) * 2 <= MAX_DOUBLE_SIZE ? (_s_) * 2 : (_s_)))


void
bytes_append(as_bytes_t *bs, char *b, size_t len, as_mem_pool_fixed_t *mp) {
  if (bs->tail != NULL && available_size(bs->tail) >= len) {
    void *dest = bytes_block_a_ptr(bs->tail);
    memcpy(dest, b, len);
  } else if (bs->head == NULL) {
    size_t nlen = len * 2 <= MAX_DOUBLE_SIZE ? len * 2 : len;
    as_bytes_block_t *block = mem_pool_fixed_alloc(
        mp, offsetof(as_bytes_block_t, d) + len);
    block->next = NULL;
    block->size = nlen;
    block->used = len;
    memcpy(block->d, b, len);

    bs->head = bs->tail = block;
    bs->cnt += 1;
    bs->size += nlen;
    bs->used += len;
  }
}
