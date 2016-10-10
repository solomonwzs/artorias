#include "mem_slot.h"

int
bits_cl0_u32(unsigned x) {
  int n = 31;
  if (x & 0xffff0000) { n -= 16;  x >>= 16; }
  if (x & 0xff00)     { n -= 8;   x >>= 8;  }
  if (x & 0xf0)       { n -= 4;   x >>= 4;  }
  if (x & 0xc)        { n -= 2;   x >>= 2;  }
  if (x & 0x2)        { n -= 1;             }
  return n;
}

int
bits_ct0_u32(unsigned x) {
  int n = 0;
  if (!(x & 0xffff))  { n += 16; x >>= 16;  }
  if (!(x & 0xff))    { n += 8;  x >>= 8;   }
  if (!(x & 0xf))     { n += 4;  x >>= 4;   }
  if (!(x & 0x3))     { n += 2;  x >>= 2;   }
  if (!(x & 0x1))     { n += 1;             }
  return n;
}

int
bits_cl0_u64(unsigned long long x) {
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
bits_ct0_u64(unsigned long long x) {
  int n = 0;
  if (!(x & 0xffffffff))  { n += 32; x >>= 32;  }
  if (!(x & 0xffff))      { n += 16; x >>= 16;  }
  if (!(x & 0xff))        { n += 8;  x >>= 8;   }
  if (!(x & 0xf))         { n += 4;  x >>= 4;   }
  if (!(x & 0x3))         { n += 2;  x >>= 2;   }
  if (!(x & 0x1))         { n += 1;             }
  return n;
}
