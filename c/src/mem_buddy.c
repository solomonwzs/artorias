#include "mem_buddy.h"

#define PARENT(_x_) (((_x_) + 1) / 2 - 1)
#define LEFT_CHILD(_x_) ((_x_) * 2 + 1)
#define RIGHT_CHILD(_x_) ((_x_) * 2 + 2)

#define MAX(_x_, _y_) ((_x_) > (_y_) ? (_x_) : (_y_))
#define IS_POWER_OF_2(_x_) (!((_x_)&((_x_)-1)))
#define NODE_SIZE(_x_) ((_x_) == 0 ? 0 : 1 << ((_x_) - 1))


static inline unsigned
fix_size(unsigned size) {
  size |= size >> 1;
  size |= size >> 2;
  size |= size >> 4;
  size |= size >> 8;
  size |= size >> 16;
  return size + 1;
}

as_buddy_t *
buddy_new(unsigned logn) {
  as_buddy_t *b;
  unsigned node_logn;
  int i;
  unsigned size = 1 << logn;

  b = (as_buddy_t *)malloc(sizeof(unsigned) +
                           (2 * size - 1) * sizeof(uint8_t));
  b->size = size * 2;
  node_logn = logn + 1;

  for (i = 0; i < 2 * size - 1; ++i) {
    if (IS_POWER_OF_2(i + 1)) {
      node_logn -= 1;
    }
    b->longest[i] = node_logn;
  }
  return b;
}

unsigned
buddy_alloc(as_buddy_t *b, int size) {
  if (b == NULL) {
    return -1;
  }

  if (size <= 0) {
    size = 1;
  } else if (!IS_POWER_OF_2(size)) {
    size = fix_size(size);
  }
  if (NODE_SIZE(b->longest[0]) < size) {
    return -1;
  }

  unsigned index = 0;
  unsigned node_size;
  for (node_size = b->size; node_size != size; node_size /= 2) {
    if (NODE_SIZE(b->longest[LEFT_CHILD(index)]) >= size) {
      index = LEFT_CHILD(index);
    } else {
      index = RIGHT_CHILD(index);
    }
  }
  b->longest[index] = 0;

  unsigned offset = (index + 1) * node_size - b->size;
  while (index) {
    index = PARENT(index);
    b->longest[index] = MAX(b->longest[LEFT_CHILD(index)],
                            b->longest[RIGHT_CHILD(index)]);
  }
  return offset;
}

void
buddy_free(as_buddy_t *b, unsigned offset) {
  if (b == NULL || offset >= b->size) {
    return;
  }

  unsigned node_size = 1;
  unsigned index = offset + b->size - 1;
  for (; index != 0 && b->longest[index]; index = PARENT(index)) {
    node_size *= 2;
  }
}

void
buddy_destroy(as_buddy_t *b) {
  free(b);
}
