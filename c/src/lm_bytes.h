#ifndef __LM_BYTES__
#define __LM_BYTES__

#include "bytes.h"
#include "lua_adapter.h"

typedef struct {
  as_bytes_t *ori;
  as_bytes_block_t *b_start;
  size_t b_offset;
  size_t len;
} as_lm_bytes_ref_t;

extern int lcf_bytes_read_from_fd(lua_State *L);

extern void inc_lm_bytes_metatable(lua_State *L);

#endif
