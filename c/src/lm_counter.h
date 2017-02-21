#ifndef __LUA_MODULE_COUNTER__
#define __LUA_MODULE_COUNTER__

typedef struct {
  int val;
} as_lm_counter_t;

typedef struct {
  as_lm_counter_t  *c;
  char          *name;
} as_lm_counter_ud_t;

#endif
