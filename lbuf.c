#include <stdio.h>
#include <stdint.h>

#include <lua.h>
#include <lauxlib.h>

// TODO:  - clean up!
//        - review int types!
//        - named fields
//        - fix rawget
//        - gc
//        - dettach buffer
//        - improve limits and other verifications
//        - implement set and rawset

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

// TODO: check limit
static uint8_t rawget(lbuf_t * lbuf, uint8_t bit_ix, uint8_t bit_len)
{
  uint8_t * buffer = lbuf->buffer;
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

// TODO: pass a free function
lbuf_t * lbuf_new(lua_State * L, void * buffer, size_t size)
{
  lbuf_t * lbuf = lua_newuserdata(L, sizeof(lbuf_t));
  lbuf->buffer = buffer;
  lbuf->size   = size;
  lbuf->align  = 8;
  luaL_getmetatable(L, "lbuf");
  lua_setmetatable(L, -2);
  return lbuf;
}

// TODO: implement lbuf.new(string | table)
static int lbuf_new_(lua_State *L)
{
  lbuf_t * lbuf = lbuf_new(L, buffer, 4);
  return 1;
}

static int lbuf_mask(lua_State *L)
{
  // TODO: create and use isudata()
  lbuf_t * lbuf = (lbuf_t *) luaL_checkudata(L, 1, "lbuf");

  if (lua_isnumber(L, 2)) {
    // TODO: when set align, erase mask table!
    lbuf->align = (uint8_t) lua_tointeger(L, 2);
    lua_pop(L, 1);
    return 1;
  }
  else if (!lua_istable(L, 2)) {
    lua_pushnil(L);
    return 1;
  }

  // TODO: create a new clean table to put masks!
  int t = 2;
  uint8_t offset = 0;
  /* table is in the stack at index 't' */
  lua_pushnil(L);  /* first key */
  while (lua_next(L, t) != 0) {
    /* uses 'key' (at index -2) and 'value' (at index -1) */
    if (!lua_isnumber(L, -1)) {
      lua_pop(L, 1);
      continue;
    }

    uint8_t bit_len = lua_tonumber(L, -1);
    if (offset + bit_len > lbuf->size * 8) { 
      lua_pop(L, 2);
      break;
    }

    lua_pushvalue(L, -2); // key
    //TODO: check if null
    mask_t * mask = lua_newuserdata(L, sizeof(mask_t)); 
    luaL_getmetatable(L, "lbuf.mask");
    lua_setmetatable(L, -2);

    mask->bit_ix  = offset;
    // TODO: also handle tables
    mask->bit_len = bit_len;
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

  // TODO: define what we should return
  lua_pushvalue(L, 1);
  return 1;
}

static int lbuf_gc(lua_State *L)
{
  // TODO: remove entry from masks table! (mt.masks[self] = nil)
  // TODO: free lbuf->buffer if ref count == 0
  return 0;
}

static int lbuf_get(lua_State *L)
{
  lbuf_t * lbuf = (lbuf_t *) luaL_checkudata(L, 1, "lbuf");
  uint8_t * buffer = (uint8_t *) lbuf->buffer;

  lua_getmetatable(L, 1);
  
  if (lua_isstring(L, 2)) {
    lua_pushvalue(L, 2);
    lua_gettable(L, -2);
    if (lua_iscfunction(L, -1))
      return 1;
    else
      lua_pop(L, 1);
  }    

  lua_getfield(L, -1, "__masks");
  lua_pushvalue(L, 1); // [ lubf | __masks | getmetatable(lbuf) | ix | lbuf ]
  lua_gettable(L, -2); // [ __masks[ lbuf ] | lbuf | __masks | (...) ]
  if (!lua_istable(L, -1)) {
    uint8_t ix = (uint8_t) luaL_checkinteger(L, 2);
    if (ix < 1 || ix * lbuf->align > lbuf->size * 8) 
      return 0;

    uint8_t value = rawget(lbuf, (ix - 1) * lbuf->align, lbuf->align);
    lua_pushnumber(L, value);
    return 1;
  }

  lua_pushvalue(L, 2); // [ ix | __masks[ lbuf ] | (...) ]
  lua_gettable(L, -2); // [ __masks[ lbuf ][ ix ] | ix | __masks[ lbuf ] | (...) ]
  if (lua_isnil(L, -1))
    return 0;

  //mask_t * mask = (mask_t *) luaL_checkudata(L, 1, "lbuf.mask");
  mask_t * mask = (mask_t *) luaL_checkudata(L, -1, "lbuf.mask");

  uint8_t value = rawget(lbuf, mask->bit_ix, mask->bit_len);
  lua_pushnumber(L, value);
  return 1;
}

#if 0
static int lbuf_rawget(lua_State *L)
{
  // TODO: use len instead align
  uint8_t ix    = (uint8_t) luaL_checkinteger(L, 1);
  uint8_t align = (uint8_t) luaL_checkinteger(L, 2);
  uint8_t value = get(buffer, 4, ix, align);
  lua_pushnumber(L, value);
  return 1;
}
#endif

static const luaL_Reg lbuf[ ] = {
  {"new", lbuf_new_},
  {"mask", lbuf_mask},
//  {"rawget", lbuf_rawget},
  {NULL, NULL}
};

static const luaL_Reg lbuf_m[ ] = {
  {"mask", lbuf_mask},
  {"__index", lbuf_get},
  {"__gc", lbuf_gc},
  {NULL, NULL}
};

static const luaL_Reg mask_m[ ] = {
  {NULL, NULL}
};

int luaopen_lbuf(lua_State *L)
{
  luaL_newmetatable(L, "lbuf");
  luaL_register(L, NULL, lbuf_m);
  lua_newtable(L);
  lua_setfield(L, -2, "__masks");
  luaL_register(L, "lbuf", lbuf);

  luaL_newmetatable(L, "lbuf.mask");
  luaL_register(L, NULL, mask_m);
  return 1;
}

