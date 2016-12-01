#include <stdlib.h>
// #include <jemalloc/jemalloc.h>
#include <stdio.h>
#include <string.h>
#include "utils.h"
#include "mem_buddy.h"

#define PARENT(_x_) (((_x_) + 1) / 2 - 1)
#define LEFT_CHILD(_x_) ((_x_) * 2 + 1)
#define RIGHT_CHILD(_x_) ((_x_) * 2 + 2)

#define MAX(_x_, _y_) ((_x_) > (_y_) ? (_x_) : (_y_))
#define IS_POWER_OF_2(_x_) (!((_x_)&((_x_)-1)))
#define NODE_SIZE(_x_) ((_x_) == 0 ? 0 : 1 << ((_x_) - 1))
#define BUDDY_SIZE(_x_) (1 << ((_x_)->logn))


static inline unsigned
fix_size(unsigned size) {
  size |= size >> 1;
  size |= size >> 2;
  size |= size >> 4;
  size |= size >> 8;
  size |= size >> 16;
  return size + 1;
}


as_mem_buddy_t *
buddy_new(uint8_t logn) {
  as_mem_buddy_t *b;
  unsigned node_logn;
  unsigned i;
  unsigned size = 1 << logn;

  b = (as_mem_buddy_t *)as_malloc(2 * size * sizeof(uint8_t));
  if (b == NULL) {
    return NULL;
  }

  b->logn = logn;
  node_logn = logn + 1;
  for (i = 0; i < 2 * BUDDY_SIZE(b) - 1; ++i) {
    if (IS_POWER_OF_2(i + 1)) {
      node_logn -= 1;
    }
    b->longest[i] = node_logn + 1;
  }
  return b;
}


int
buddy_alloc(as_mem_buddy_t *b, unsigned size) {
  if (b == NULL) {
    return -1;
  }

  if (!IS_POWER_OF_2(size)) {
    size = fix_size(size);
  }
  if (NODE_SIZE(b->longest[0]) < size) {
    return -1;
  }

  unsigned index = 0;
  unsigned node_size;
  for (node_size = BUDDY_SIZE(b); node_size != size; node_size /= 2) {
    if (NODE_SIZE(b->longest[LEFT_CHILD(index)]) >= size) {
      index = LEFT_CHILD(index);
    } else {
      index = RIGHT_CHILD(index);
    }
  }
  b->longest[index] = 0;

  unsigned offset = (index + 1) * node_size - BUDDY_SIZE(b);
  while (index) {
    index = PARENT(index);
    b->longest[index] = MAX(b->longest[LEFT_CHILD(index)],
                            b->longest[RIGHT_CHILD(index)]);
  }
  return offset;
}


void
buddy_free(as_mem_buddy_t *b, unsigned offset) {
  if (b == NULL || offset >= BUDDY_SIZE(b)) {
    return;
  }

  uint8_t node_logn = 1;
  unsigned index = offset + BUDDY_SIZE(b) - 1;
  for (; b->longest[index]; index = PARENT(index)) {
    node_logn += 1;
    if (index == 0) {
      return;
    }
  }
  b->longest[index] = node_logn;

  unsigned left_longest;
  unsigned right_longest;
  while (index) {
    index = PARENT(index);
    node_logn += 1;

    left_longest = b->longest[LEFT_CHILD(index)];
    right_longest = b->longest[RIGHT_CHILD(index)];
    if (left_longest == right_longest && left_longest == node_logn) {
      b->longest[index] = node_logn;
    } else {
      b->longest[index] = MAX(left_longest, right_longest);
    }
  }
}

void
buddy_print(as_mem_buddy_t *b) {
  if (b == NULL || BUDDY_SIZE(b) > 64) {
    return;
  }

  char m[65];
  unsigned i;
  unsigned j;
  unsigned node_size;
  unsigned offset;

  memset(m, '_', sizeof(m));
  node_size = BUDDY_SIZE(b) * 2;
  for (i = 0; i < 2 * BUDDY_SIZE(b) - 1; ++i) {
    if (IS_POWER_OF_2(i + 1)) {
      node_size /= 2;
    }

    if (b->longest[i] == 0) {
      if (i >= BUDDY_SIZE(b) - 1) {
        m[i - BUDDY_SIZE(b) + 1] = '*';
      } else if (b->longest[LEFT_CHILD(i)] && b->longest[RIGHT_CHILD(i)]){
        offset = (i + 1) * node_size - BUDDY_SIZE(b);
        for (j = offset; j < offset + node_size; ++j) {
          m[j] = '*';
        }
      }
    }
  }
  m[BUDDY_SIZE(b)] = '\0';
  puts(m);
}


void
buddy_destroy(as_mem_buddy_t *b) {
  as_free(b);
}
