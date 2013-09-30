#include <stdio.h>
#include <stdint.h>
#include <stdbool.h>

#include <lua.h>
#include <lauxlib.h>

uint8_t buffer[ 4 ] = { 1 , 2, 4, 8 };

typedef struct {
  uint8_t align;
  void *  buffer;
  size_t  size;
} lbuf_t;

typedef struct {
  uint8_t bit_ix;
  uint8_t bit_len;
} mask_t;

uint8_t get(uint8_t * buffer, size_t n, uint8_t ix, uint8_t align)
{
  uint8_t bit_ix  = (ix - 1) * align;
  uint8_t byte_ix = bit_ix / 8;

  uint8_t bit_end  = bit_ix + align - 1;
  uint8_t bit_upper_limit = 8 * (byte_ix + 1);
  uint8_t overflow = bit_end >= bit_upper_limit ? (bit_end % bit_upper_limit) + 1 : 0;
  uint8_t left_shift = !overflow ? (8 - (bit_ix % 8) - align) : 0;
  uint8_t mask     = ((uint8_t) -1 >> (8 - (align - overflow))) << left_shift;

  uint8_t result = (uint8_t) buffer[ byte_ix ] & mask;

  if (overflow)
    result = (result << overflow) | (buffer[ byte_ix + 1 ] >> (8 - overflow));
  else
    result = result >> left_shift;

#ifdef DEBUG
  printf("[ ix = %d, align = %d]\n", ix, align);
  printf("bit_ix   = %d\n", (int) bit_ix);
  printf("bit_end  = %d\n", (int) bit_end);
  printf("byte_ix  = %d\n", (int) byte_ix);
  printf("overflow = %d\n", (int) overflow);
  printf("mask [1] = %d\n", (int) (8 - (align - overflow)));
  printf("mask [2] = %d\n", (int) left_shift);
  printf("mask     = %X\n", (int) mask);
  printf("result   = %d\n", (int) result);
  printf("----------------------\n\n");
#endif
  return result;
}

uint8_t get_(uint8_t * buffer, size_t n, uint8_t bit_ix, uint8_t bit_len)
{
  uint8_t byte_ix = bit_ix / 8;

  uint8_t bit_end  = bit_ix + bit_len - 1;
  uint8_t bit_upper_limit = 8 * (byte_ix + 1);
  uint8_t overflow = bit_end >= bit_upper_limit ? (bit_end % bit_upper_limit) + 1 : 0;
  uint8_t left_shift = !overflow ? (8 - (bit_ix % 8) - bit_len) : 0;
  uint8_t mask     = ((uint8_t) -1 >> (8 - (bit_len - overflow))) << left_shift;

  uint8_t result = (uint8_t) buffer[ byte_ix ] & mask;

  if (overflow)
    result = (result << overflow) | (buffer[ byte_ix + 1 ] >> (8 - overflow));
  else
    result = result >> left_shift;
  return result;
}

static int lbuf_new(lua_State *L)
{
  lbuf_t * lbuf = lua_newuserdata(L, sizeof(lbuf_t));
  lbuf->buffer = (void *) buffer;
  lbuf->align = 8;
  lbuf->size = 8 * 4;
  luaL_getmetatable(L, "lbuf");
  lua_setmetatable(L, -2);
  return 1;
}

// TODO: OO access to mask
static int lbuf_mask(lua_State *L)
{
  lbuf_t * lbuf = (lbuf_t *) luaL_checkudata(L, 1, "lbuf");

  lbuf->align = (uint8_t) luaL_checkinteger(L, 2);

  return 0;
}

static int lbuf_maskt(lua_State *L)
{
  lbuf_t * lbuf = (lbuf_t *) luaL_checkudata(L, 1, "lbuf");

  if (!lua_istable(L, 2)) {
    // TODO: return nil?
    lua_pushnumber(L, false);
    return 1;
  }

  int t = 2;
  uint8_t offset = 0;
  /* table is in the stack at index 't' */
  lua_pushnil(L);  /* first key */
  while (lua_next(L, t) != 0) {
    /* uses 'key' (at index -2) and 'value' (at index -1) */
    printf("%s - %s\n",
        lua_typename(L, lua_type(L, -2)),
        lua_typename(L, lua_type(L, -1)));

    if (!lua_isnumber(L, -1))
      continue;

    lua_pushvalue(L, -2); // key

    mask_t * mask = lua_newuserdata(L, sizeof(mask_t));
    mask->bit_ix  = offset;
    mask->bit_len = lua_tonumber(L, -3); // TODO: checknumber
    offset        = mask->bit_ix + mask->bit_len;

    lua_settable(L, 2); // mask_table[ k ] = mask

    /* removes 'value'; keeps 'key' for next iteration */
    lua_pop(L, 1);
  }

  lua_getmetatable(L, 1);
  lua_getfield(L, -1, "__masks");
  lua_pushvalue(L, 1);
  lua_pushvalue(L, 2);
  lua_settable(L, -3); 

  // TODO: return usertadum itself?
  lua_pushnumber(L, true);
  return 1;
}

static int lbuf_gc(lua_State *L)
{
  // TODO: remove entry from masks table! (mt.masks[self] = nil)
  return 0;
}

static int lbuf_get(lua_State *L)
{
  lbuf_t * lbuf = (lbuf_t *) luaL_checkudata(L, 1, "lbuf");
  uint8_t * buffer = (uint8_t *) lbuf->buffer;

  uint8_t ix = (uint8_t) luaL_checkinteger(L, 2);
  if (ix < 1 || ix * lbuf->align > lbuf->size) 
    return 0;

  uint8_t value = get(buffer, 4, ix, lbuf->align);
  lua_pushnumber(L, value);
  return 1;
}

static int lbuf_get_(lua_State *L)
{
  lbuf_t * lbuf = (lbuf_t *) luaL_checkudata(L, 1, "lbuf");
  uint8_t * buffer = (uint8_t *) lbuf->buffer;

#if 0
  uint8_t ix = (uint8_t) luaL_checkinteger(L, 2);
  if (ix < 1 || ix * lbuf->align > lbuf->size) 
    return 0;
#endif

  lua_getmetatable(L, 1);
  lua_getfield(L, -1, "__masks");
  lua_pushvalue(L, 1); // [ lubf | __masks | getmetatable(lbuf) | ix | lbuf ]
  lua_gettable(L, -2); // [ __masks[ lubf ] | lubf | __masks | (...) ]
  lua_pushvalue(L, 2); // [ ix | __masks[ lubf ] | (...) ]
  lua_gettable(L, -2); // [ __masks[ lubf ][ ix ] | ix | __masks[ lubf ] | (...) ]
  if (lua_isnil(L, -1))
    return 0;

  //mask_t * mask = (mask_t *) luaL_checkudata(L, 1, "lbuf.mask");
  mask_t * mask = (mask_t *) lua_touserdata(L, -1);

  uint8_t value = get_(buffer, 4, mask->bit_ix, mask->bit_len);
  lua_pushnumber(L, value);
  return 1;
}

static int lbuf_rawget(lua_State *L)
{
  // TODO: use len instead align
  uint8_t ix    = (uint8_t) luaL_checkinteger(L, 1);
  uint8_t align = (uint8_t) luaL_checkinteger(L, 2);
  uint8_t value = get(buffer, 4, ix, align);
  lua_pushnumber(L, value);
  return 1;
}

static const luaL_Reg lbuf[ ] = {
  {"new", lbuf_new},
  {"get", lbuf_get},
  {"mask", lbuf_mask},
  {"maskt", lbuf_maskt},
  {"rawget", lbuf_rawget},
  {NULL, NULL}
};

static const luaL_Reg lbuf_m[ ] = {
  {"mask", lbuf_mask},
  {"__index", lbuf_get_},
  {"__gc", lbuf_gc},
  {NULL, NULL}
};

int luaopen_lbuf(lua_State *L)
{
  luaL_newmetatable(L, "lbuf");
  luaL_register(L, NULL, lbuf_m);
  lua_newtable(L);
  lua_setfield(L, -2, "__masks");
  luaL_register(L, "lbuf", lbuf);
  return 1;
}

