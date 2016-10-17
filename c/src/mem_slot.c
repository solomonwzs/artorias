#include "mem_slot.h"

#define SLOT_SIZE(_x_) ((_x_)->n * 8)

#define as_malloc malloc
#define as_free free


int
bits_cl0_u8(uint8_t x) {
  int n = 7;
  if (x & 0xf0)       { n -= 4;   x >>= 4;  }
  if (x & 0xc)        { n -= 2;   x >>= 2;  }
  if (x & 0x2)        { n -= 1;             }
  return n;
}


int
bits_ct0_u8(uint8_t x) {
  int n = 0;
  if (!(x & 0xf))     { n += 4;  x >>= 4;   }
  if (!(x & 0x3))     { n += 2;  x >>= 2;   }
  if (!(x & 0x1))     { n += 1;             }
  return n;
}


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
  s = (as_mem_slot_t *)as_malloc(sizeof(as_mem_slot_t) + n - 1);
  if (s == NULL) {
    return NULL;
  }
  s->n = n;
  bzero((char *)s->flag, n);

  return s;
}


void
slot_destroy(as_mem_slot_t *s) {
  as_free(s);
}


static inline void
mark_used_flag(uint8_t *flag, int offset, int len) {
  int offset0 = offset + len;
  int i = offset / 8;
  int i0 = offset0 / 8;

  if (i == i0) {
    flag[i] |= (0xff >> (offset % 8)) & (0xff << (8 - offset0 % 8));
  } else {
    flag[i] |= (0xff >> (offset % 8));
    flag[i0] |= (0xff << (8 - offset0 % 8));
    int j;
    for (j = i + 1; j < i0; ++j) {
      flag[j] = 0xff;
    }
  }
}


static inline void
mark_free_flag(uint8_t *flag, int offset, int len) {
  int offset0 = offset + len;
  int i = offset / 8;
  int i0 = offset0 / 8;

  if (i == i0) {
    flag[i] &= ~((0xff >> (offset % 8)) & (0xff << (8 - offset0 % 8)));
  } else {
    flag[i] &= ~(0xff >> (offset % 8));
    flag[i0] &= ~(0xff << (8 - offset0 % 8));
    int j;
    for (j = i + 1; j < i0; ++j) {
      flag[j] = 0;
    }
  }
}


int
slot_alloc(as_mem_slot_t *s, unsigned size) {
  if (s == NULL || size > SLOT_SIZE(s) || size == 0) {
    return -1;
  }

  unsigned i = 0;
  int offset = 0;
  unsigned empty = 0;
  int leading = 0;
  for (i = 0; i < s->n && empty < size; ++i) {
    if (s->flag[i] == 0) {
      empty += 8;
    } else {
      leading = bits_cl0_u8(s->flag[i]);
      if (empty + leading >= size) {
        empty += leading;
      } else {
        empty = bits_ct0_u8(s->flag[i]);
        offset = i * 8 + (8 - empty);
      }
    }
  }

  if (empty >= size) {
    mark_used_flag(s->flag, offset, size);
    return offset;
  }
  return -1;
}


void
slot_free(as_mem_slot_t *s, int offset, unsigned size) {
  if (s == NULL || offset > SLOT_SIZE(s) || offset + size > SLOT_SIZE(s)) {
    return;
  }
  mark_free_flag(s->flag, offset, size);
}


void
slot_print(as_mem_slot_t *s) {
  if (s == NULL) {
    return;
  }

  uint8_t i;
  uint8_t j;
  uint8_t k;
  for (i = 0; i < s->n; ++i) {
    k = 1 << 7;
    for (j = 0; j < 8; ++j) {
      if (s->flag[i] & k) {
        printf("*");
      } else {
        printf("_");
      }
      k = k >> 1;
    }
  }
  printf("\n");
}
