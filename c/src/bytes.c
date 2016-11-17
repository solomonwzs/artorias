#include <unistd.h>
#include <string.h>
#include "utils.h"
#include "bytes.h"

#define MAX_BLOCK_SIZE 1024
#define MIN_BLOCK_SIZE 16

#define available_size(_n_) ((_n_)->size - (_n_)->used)

#define bs_block_a_ptr(_n_) ((void *)(&(_n_)->d[(_n_)->used]))

#define new_block_size(_s_) \
    (offsetof(as_bytes_block_t, d) + \
     ((_s_) * 2 <= MAX_DOUBLE_SIZE ? (_s_) * 2 : (_s_)))

#define to_block(_d_) (container_of(_d_, as_bytes_block_t, d))


void
bytes_reset_used(as_bytes_t *bs, size_t n) {
  bs->used = bs->size > n ? n : bs->size;
  as_bytes_block_t *b = bs->head;
  while (b != NULL && n > 0) {
    if (n <= b->used) {
      b->used = n;
      n = 0;
    } else {
      n -= b->used;
      b->used = b->size;
    }
    bs->curr = b;
    b = b->next;
  }
  while (b != NULL) {
    b->used = 0;
    b = b->next;
  }
}


void
bytes_init(as_bytes_t *bs, as_mem_pool_fixed_t *mp) {
  bs->size = bs->used = 0;
  bs->head = bs->curr = NULL;
  bs->cnt = 0;
  bs->used = 0;
  bs->mp = mp;
}


int
bytes_append(as_bytes_t *bs, const void *src, size_t n) {
  size_t size;
  size_t oused = bs->used;

  as_bytes_block_t **bc = &bs->curr;
  while (*bc != NULL) {
    size = available_size(*bc);
    if (n <= size) {
      memcpy(bs_block_a_ptr(*bc), src, n);
      bs->used += n;
      (*bc)->used += n;
      return 0;
    } else {
      memcpy(bs_block_a_ptr(*bc), src, size);
      src = &((char *)src)[size];
      n -= size;
      bs->used += size;
      (*bc)->used += size;
      bc = &(*bc)->next;
    }
  }

  size = n * 2 < MAX_BLOCK_SIZE ? n * 2 : n;
  if (size < MIN_BLOCK_SIZE) {
    size = MIN_BLOCK_SIZE;
  }
  as_bytes_block_t *b;
  b = mpf_alloc(bs->mp, offsetof(as_bytes_block_t, d) + size);
  if (b == NULL) {
    bytes_reset_used(bs, oused);
    return -1;
  }
  b->size = size;
  b->used = n;
  b->next = NULL;
  memcpy(b->d, src, n);
  
  bs->size += size;
  bs->used += n;
  bs->cnt += 1;
  *bc = b;
  if (bs->head == NULL) {
    bs->head = b;
  }
  return 0;
}


ssize_t
bytes_write_to_fd(int fd, as_bytes_t *bs, size_t nbyte) {
  as_bytes_block_t *b = bs->head;
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
bytes_destroy(as_bytes_t *bs) {
  as_bytes_block_t *b = bs->head;
  while (b != NULL) {
    as_bytes_block_t *tmp = b->next;
    mpf_recycle(b);
    b = tmp;
  }
  bs->size = 0;
  bs->used = 0;
  bs->cnt = 0;
  bs->head = bs->curr = NULL;
}


void
bytes_print(as_bytes_t *bs) {
  as_bytes_block_t *b = bs->head;
  while (b != NULL) {
    printf("[(%d/%d)", b->used, b->size);
    for (size_t i = 0; i < b->used; ++i) {
      printf(" %c", b->d[i]);
    }
    printf("]");
    b = b->next;
  }
  printf("\n");
}


int
bytes_read_from_fd(as_bytes_t *bs, int fd) {
  // char buf[MAX_BLOCK_SIZE];
  // int nbyte;
  int n = 0;
  return n;
}
