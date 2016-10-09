#ifndef __MEM_BUDDY__
#define __MEM_BUDDY__

#include <stdlib.h>
#include <stdio.h>
#include <stdint.h>
#include <string.h>

typedef struct {
  uint8_t logn;
  uint8_t longest[1];
} as_buddy_t;

extern as_buddy_t *
buddy_new(uint8_t logn);

extern int
buddy_alloc(as_buddy_t *b, unsigned size);

extern void
buddy_free(as_buddy_t *b, unsigned offset);

extern void
buddy_destroy(as_buddy_t *b);

extern void
buddy_print(as_buddy_t *b);

#endif
