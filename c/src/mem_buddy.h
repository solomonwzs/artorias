#ifndef __MEM_BUDDY__
#define __MEM_BUDDY__

#include <stdlib.h>
#include <stdint.h>

typedef struct {
  unsigned size;
  uint8_t longest[0];
} as_buddy_t;

extern as_buddy_t *
buddy_new(unsigned logn);

extern unsigned
buddy_alloc(as_buddy_t *b, int size);

extern void
buddy_free(as_buddy_t *b, unsigned offset);

extern void
buddy_destroy(as_buddy_t *b);

#endif
