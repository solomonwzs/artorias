#ifndef __MEM_SLOT
#define __MEM_SLOT

#include <stdint.h>
#include <stdlib.h>
#include <string.h>

#ifdef __GNUC__
#define bits_cl0_u32 __builtin_clz
#define bits_ct0_u32 __builtin_ctz
#define bits_cl0_u64 __builtin_clzll
#define bits_ct0_u64 __builtin_ctzll
#else
extern int
__bits_cl0_u32(unsigned x);

extern int
__bits_ct0_u32(unsigned x);

extern int
__bits_cl0_u64(unsigned long long x);

extern int
__bits_ct0_u64(unsigned long long x);

#define bits_cl0_u32 __bits_cl0_u32
#define bits_ct0_u32 __bits_ct0_u32
#define bits_cl0_u64 __bits_cl0_u64
#define bits_ct0_u64 __bits_cl0_u64
#endif

typedef struct {
  uint8_t n;
  uint8_t flag[1];
} as_mem_slot_t;

extern as_mem_slot_t *
slot_new(uint8_t n);

void
slot_destroy(as_mem_slot_t *s);

#endif
