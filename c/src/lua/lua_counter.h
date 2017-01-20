#ifndef __LUA_COUNTER__
#define __LUA_COUNTER__

typedef struct {
  int val;
} as_counter_t;

typedef struct {
  as_counter_t  *c;
  char          *name;
} as_counter_ud_t;

#endif
