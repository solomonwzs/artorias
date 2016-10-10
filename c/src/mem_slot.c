#include "mem_slot.h"

#define SLOT_SIZE(_x_) ((_x_)->n * 8)

int
__bits_cl0_u32(unsigned x) {
  int n = 31;
  if (x & 0xffff0000) { n -= 16;  x >>= 16; }
  if (x & 0xff00)     { n -= 8;   x >>= 8;  }
  if (x & 0xf0)       { n -= 4;   x >>= 4;  }
  if (x & 0xc)        { n -= 2;   x >>= 2;  }
  if (x & 0x2)        { n -= 1;             }
  return n;
}

int
__bits_ct0_u32(unsigned x) {
  int n = 0;
  if (!(x & 0xffff))  { n += 16; x >>= 16;  }
  if (!(x & 0xff))    { n += 8;  x >>= 8;   }
  if (!(x & 0xf))     { n += 4;  x >>= 4;   }
  if (!(x & 0x3))     { n += 2;  x >>= 2;   }
  if (!(x & 0x1))     { n += 1;             }
  return n;
}

int
__bits_cl0_u64(unsigned long long x) {
  int n = 63;
  if (x & 0xffffffff00000000) { n -= 32;  x >>= 32; }
  if (x & 0xffff0000)         { n -= 16;  x >>= 16; }
  if (x & 0xff00)             { n -= 8;   x >>= 8;  }
  if (x & 0xf0)               { n -= 4;   x >>= 4;  }
  if (x & 0xc)                { n -= 2;   x >>= 2;  }
  if (x & 0x2)                { n -= 1;             }
  return n;
}

int
__bits_ct0_u64(unsigned long long x) {
  int n = 0;
  if (!(x & 0xffffffff))  { n += 32; x >>= 32;  }
  if (!(x & 0xffff))      { n += 16; x >>= 16;  }
  if (!(x & 0xff))        { n += 8;  x >>= 8;   }
  if (!(x & 0xf))         { n += 4;  x >>= 4;   }
  if (!(x & 0x3))         { n += 2;  x >>= 2;   }
  if (!(x & 0x1))         { n += 1;             }
  return n;
}

as_mem_slot_t *
slot_new(uint8_t n) {
  as_mem_slot_t *s;
  s = (as_mem_slot_t *)malloc(
      sizeof(as_mem_slot_t) + n - 1);
  if (s == NULL) {
    return NULL;
  }
  s->n = n;
  bzero((char *)s->flag, n);

  return s;
}

void
slot_destroy(as_mem_slot_t *s) {
  free(s);
}

static inline void
mark_used_flag(uint8_t *flag, int offset, int len) {
  int i = offset / 8;
  int j = offset % 8;

  flag[i] |= 0xff >> j;
  for (int k = 0; k < len - j; ) {
  }
}

int
slot_alloc(as_mem_slot_t *s, unsigned size) {
  if (size > SLOT_SIZE(s)) {
    return -1;
  }
  uint32_t *p;
  unsigned i = 0;
  unsigned empty = 0;
  int leading = 0;
  int trailing = 0;
  int offset = 0;
  while (i + 4 < s->n) {
    p = (uint32_t *)&(s->flag[i]);
    if (p == 0) {
      empty += 32;
      if (empty >= size) {
        break;
      }
      i += 4;
    }
  }

  if (empty >= size) {
  }
  return offset;
}
