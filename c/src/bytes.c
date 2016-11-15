#include <unistd.h>
#include <string.h>
#include "utils.h"
#include "bytes.h"

#define MAX_DOUBLE_SIZE 512

#define available_size(_n_) ((_n_)->size - (_n_)->used)
#define bytes_block_a_ptr(_n_) ((void *)(&(_n_)->d[(_n_)->used]))
#define new_block_size(_s_) \
    (offsetof(as_bytes_block_t, d) + \
     ((_s_) * 2 <= MAX_DOUBLE_SIZE ? (_s_) * 2 : (_s_)))


void
bytes_append(as_bytes_t *buf, void *b, size_t len, as_mem_pool_fixed_t *mp) {
  size_t a_size;
  size_t nlen = 0;
  if (buf->head == NULL) {
    size_t nlen = len * 2 <= MAX_DOUBLE_SIZE ? len * 2 : len;
    as_bytes_block_t *block = mem_pool_fixed_alloc(
        mp, offsetof(as_bytes_block_t, d) + nlen);
    block->next = NULL;
    block->size = nlen;
    block->used = len;
    memcpy(block->d, b, len);

    buf->head = buf->tail = block;
    buf->cnt += 1;
  } else if ((a_size = available_size(buf->tail)) < len) {
    void *dest = bytes_block_a_ptr(buf->tail);
    memcpy(dest, b, a_size);
    buf->tail->used += a_size;

    size_t rl = len - a_size;
    size_t nlen = rl + len <= MAX_DOUBLE_SIZE ? rl + len : rl;
    as_bytes_block_t *block = mem_pool_fixed_alloc(
        mp, offsetof(as_bytes_block_t, d) + nlen);
    block->next = NULL;
    block->size = nlen;
    block->used = rl;
    memcpy(block->d, &((char *)b)[a_size], rl);

    buf->tail->next = block;
    buf->tail = block;
    buf->cnt += 1;
  } else {
    void *dest = bytes_block_a_ptr(buf->tail);
    memcpy(dest, b, len);
    buf->tail->used += len;
  }
  buf->size += nlen;
  buf->used += len;
}


ssize_t
bytes_write_to_fd(int fd, as_bytes_t *buf, size_t nbyte) {
  as_bytes_block_t *b = buf->head;
  ssize_t n = 0;
  ssize_t ret;
  while (b != NULL && nbyte > 0) {
    if (nbyte < b->used) {
      ret = write(fd, b->d, nbyte);
      if (ret == -1) {
        debug_perror("bytes write");
        return -1;
      } else {
        return n + ret;
      }
    } else {
      ret = write(fd, b->d, b->used);
      if (ret == -1) {
        debug_perror("bytes write");
        return -1;
      } else {
        nbyte -= b->used;
        n += ret;
        b = b->next;
      }
    }
  }
  return n;
}


void
bytes_destroy(as_bytes_t *buf) {
  as_bytes_block_t *b = buf->head;
  while (b != NULL) {
    as_bytes_block_t *tmp = b->next;
    mem_pool_fixed_recycle(b);
    b = tmp;
  }
  buf->size = 0;
  buf->used = 0;
  buf->cnt = 0;
  buf->head = buf->tail = NULL;
}


void
bytes_print(as_bytes_t *buf) {
  as_bytes_block_t *b = buf->head;
  while (b != NULL) {
    printf("[");
    for (size_t i = 0; i < b->used; ++i) {
      printf(" %d", b->d[i]);
    }
    printf("]");
    b = b->next;
  }
  printf("\n");
}
