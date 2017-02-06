#include <lua.h>
#include <lauxlib.h>
#include "lcounter.h"
#include "../lua_utils.h"
#include "../utils.h"


static as_counter_t *
counter_create(int start) {
  as_counter_t *c = as_malloc(sizeof(as_counter_t));
  if (c == NULL) {
    return NULL;
  }

  c->val = start;
  return c;
}


static void
counter_destroy(as_counter_t *c) {
  if (c != NULL) {
    as_free(c);
  }
}


static void
counter_add(as_counter_t *c, int amount) {
  if (c != NULL) {
    c->val += amount;
  }
}


static int
counter_getval(as_counter_t *c) {
  return c != NULL ? c->val : 0;
}


// [-2, +1, e]
static int
lcf_counter_new(lua_State *L) {
  as_counter_ud_t *cu;
  const char *name;
  int start;

  start = luaL_checkinteger(L, 1);
  name = luaL_checkstring(L, 2);

  if (name == NULL) {
    luaL_error(L, "name can not be empty");
  }

  cu = (as_counter_ud_t *)lua_newuserdata(L, sizeof(as_counter_ud_t));
  cu->c = NULL;
  cu->name = NULL;

  luaL_getmetatable(L, LM_COUNTER);
  lua_setmetatable(L, -2);

  cu->c = counter_create(start);
  cu->name = strdup(name);

  return 1;
}


// [-0, +0, e]
static int
lcf_counter_add(lua_State *L) {
  as_counter_ud_t *cu;
  int amount;

  cu = (as_counter_ud_t *)luaL_checkudata(L, 1, LM_COUNTER);
  amount = luaL_checkinteger(L, 2);
  counter_add(cu->c, amount);

  return 0;
}


// [-0, +1, e]
static int
lcf_counter_getval(lua_State *L) {
  as_counter_ud_t *cu;

  cu = (as_counter_ud_t *)luaL_checkudata(L, 1, LM_COUNTER);
  lua_pushinteger(L, counter_getval(cu->c));

  return 1;
}


// [-0, +1, e]
static int
lcf_counter_getname(lua_State *L) {
  as_counter_ud_t *cu;

  cu = (as_counter_ud_t *)luaL_checkudata(L, 1, LM_COUNTER);
  lua_pushstring(L, cu->name);

  return 1;
}


// [-0, +0, e]
static int
lcf_counter_destroy(lua_State *L) {
  as_counter_ud_t *cu;
  cu = (as_counter_ud_t *)luaL_checkudata(L, 1, LM_COUNTER);
  counter_destroy(cu->c);
  cu->c = NULL;

  if (cu->name != NULL) {
    as_free(cu->name);
    cu->name = NULL;
  }

  return 0;
}


// [-0, +0, e]
static int
lcf_counter_tostring(lua_State *L) {
  as_counter_ud_t *cu;

  cu = (as_counter_ud_t *)luaL_checkudata(L, 1, LM_COUNTER);
  lua_pushfstring(L, "%s(%d)", cu->name, counter_getval(cu->c));

  return 1;
}


static const struct luaL_Reg as_l_counter_methods[] = {
  {"add", lcf_counter_add},
  {"getval", lcf_counter_getval},
  {"getname", lcf_counter_getname},
  {"__gc", lcf_counter_destroy},
  {"__tostring", lcf_counter_tostring},
  {NULL, NULL},
};


static const struct luaL_Reg as_l_counter_functions[] = {
  {"new", lcf_counter_new},
  {NULL, NULL},
};


int luaopen_lcounter(lua_State *L) {
  luaL_newmetatable(L, LM_COUNTER);
  lua_pushvalue(L, -1);
  lua_setfield(L, -2, "__index");
  luaL_setfuncs(L, as_l_counter_methods, 0);
  luaL_newlib(L, as_l_counter_functions);

  return 1;
}
