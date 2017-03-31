#include "lm_counter.h"
#include "mem_pool.h"
#include "lua_utils.h"
#include "utils.h"


static as_lm_counter_t *
counter_create(int start, as_mem_pool_fixed_t *mp) {
  if (mp == NULL) {
    return NULL;
  }
  as_lm_counter_t *c = mpf_alloc(mp, sizeof(as_lm_counter_t));
  if (c == NULL) {
    return NULL;
  }

  c->val = start;
  return c;
}


static void
counter_destroy(as_lm_counter_t *c) {
  if (c != NULL) {
    mpf_recycle(c);
  }
}


static void
counter_add(as_lm_counter_t *c, int amount) {
  if (c != NULL) {
    c->val += amount;
  }
}


static int
counter_getval(as_lm_counter_t *c) {
  return c != NULL ? c->val : 0;
}


// [-2, +1, e]
static int
lcf_counter_new(lua_State *L) {
  as_lm_counter_ud_t *cu;
  const char *name;
  int start;

  start = luaL_checkinteger(L, 1);
  name = luaL_checkstring(L, 2);

  if (name == NULL) {
    luaL_error(L, "name can not be empty");
  }

  lua_pushstring(L, LRK_MEM_POOL);
  lua_gettable(L, LUA_REGISTRYINDEX);
  as_mem_pool_fixed_t *mp = (as_mem_pool_fixed_t *)lua_touserdata(L, -1);

  cu = (as_lm_counter_ud_t *)lua_newuserdata(L, sizeof(as_lm_counter_ud_t));
  cu->c = NULL;
  cu->name = NULL;

  luaL_getmetatable(L, LM_COUNTER);
  lua_setmetatable(L, -2);

  cu->c = counter_create(start, mp);
  cu->name = strdup(name);

  return 1;
}


// [-0, +0, e]
static int
lcf_counter_add(lua_State *L) {
  as_lm_counter_ud_t *cu = (as_lm_counter_ud_t *)luaL_checkudata(
      L, 1, LM_COUNTER);
  int amount = luaL_checkinteger(L, 2);

  counter_add(cu->c, amount);

  return 0;
}


// [-0, +1, e]
static int
lcf_counter_getval(lua_State *L) {
  as_lm_counter_ud_t *cu = (as_lm_counter_ud_t *)luaL_checkudata(
      L, 1, LM_COUNTER);

  lua_pushinteger(L, counter_getval(cu->c));

  return 1;
}


// [-0, +1, e]
static int
lcf_counter_getname(lua_State *L) {
  as_lm_counter_ud_t *cu = (as_lm_counter_ud_t *)luaL_checkudata(
      L, 1, LM_COUNTER);

  lua_pushstring(L, cu->name);

  return 1;
}


// [-0, +0, e]
static int
lcf_counter_destroy(lua_State *L) {
  as_lm_counter_ud_t *cu = (as_lm_counter_ud_t *)luaL_checkudata(
      L, 1, LM_COUNTER);

  counter_destroy(cu->c);
  cu->c = NULL;
  if (cu->name != NULL) {
    as_free(cu->name);
    cu->name = NULL;
  }

  return 0;
}


// [-0, +1, e]
static int
lcf_counter_tostring(lua_State *L) {
  as_lm_counter_ud_t *cu = (as_lm_counter_ud_t *)luaL_checkudata(
      L, 1, LM_COUNTER);
  lua_pushfstring(L, "%s(%d)", cu->name, counter_getval(cu->c));
  return 1;
}


static const struct luaL_Reg
as_lm_counter_methods[] = {
  {"add", lcf_counter_add},
  {"getval", lcf_counter_getval},
  {"getname", lcf_counter_getname},
  {"__gc", lcf_counter_destroy},
  {"__tostring", lcf_counter_tostring},
  {NULL, NULL},
};


static const struct luaL_Reg
as_lm_counter_functions[] = {
  {"new", lcf_counter_new},
  {NULL, NULL},
};


int
luaopen_lm_counter(lua_State *L) {
  // luaL_newmetatable(L, LM_COUNTER);
  // lua_pushvalue(L, -1);
  // lua_setfield(L, -2, "__index");
  // luaL_setfuncs(L, as_lm_counter_methods, 0);
  // luaL_newlib(L, as_lm_counter_functions);

  aluaL_newmetatable_with_methods(L, LM_COUNTER, as_lm_counter_methods);
  aluaL_newlib(L, "lm_counter", as_lm_counter_functions);

  return 1;
}
