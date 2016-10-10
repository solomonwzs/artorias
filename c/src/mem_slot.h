#ifndef __MEM_SLOT
#define __MEM_SLOT

#ifdef __GNUC__
#define bits_cl0_u32 __builtin_clz
#define bits_ct0_u32 __builtin_ctz
#define bits_cl0_u64 __builtin_clzll
#define bits_ct0_u64 __builtin_ctzll
#else
extern int
bits_cl0_u32(unsigned x);

extern int
bits_ct0_u32(unsigned x);

extern int
bits_cl0_u64(unsigned long long x);

extern int
bits_ct0_u64(unsigned long long x);
#endif

#endif
