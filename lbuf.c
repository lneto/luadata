/*
* Copyright (C) 2013 Lourival Vieira Neto <lourival.neto@gmail.com>.
* All rights reserved.
*
* Permission is hereby granted, free of charge, to any person obtaining a copy
* of this software and associated documentation files (the "Software"), to deal
* in the Software without restriction, including without limitation the rights
* to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
* copies of the Software, and to permit persons to whom the Software is
* furnished to do so, subject to the following conditions:
* 
* The above copyright notice and this permission notice shall be included in
* all copies or substantial portions of the Software.
* 
* THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
* IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
* FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT.  IN NO EVENT SHALL THE
* AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
* LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
* OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN
* THE SOFTWARE.
*/

#include <stdio.h>
#include <stdint.h>

#include <lua.h>
#include <lauxlib.h>

#define DEBUG

// TODO:  - clean up!
//        - review int types!
//        - gc
//        - improve limits and other verifications
// NEXT_VERSION:
//        - named fields
//        - implement set and rawset

uint8_t buffer[ 4 ] = { 0xFF, 0x7F, 0x80, 0x00 };

typedef struct {
  uint8_t alignment;
  void *  buffer;
  size_t  size;
} lbuf_t;

typedef struct {
// TODO: use two ix levels (bit and align (e.g., byte))?
  uint32_t offset;
  uint8_t  length;
} mask_t;

// TODO: move these macros and rawget to a utils file?
#define NUM_BITS_SECOND_POS(alignment, mask, overflow) \
  (overflow ? 0 : (alignment - (mask->offset % alignment) - mask->length)); \

#define COMPL_NUM_BITS_FIRST_POS(alignment, mask, overflow) \
   (alignment - (mask->length - overflow)); \

#define APPLY_MASK(type, lbuf, mask, buffer_pos, overflow, result) \
  do { \
    type *  buffer  = (type *) lbuf->buffer;                               \
    uint8_t rshift  = COMPL_NUM_BITS_FIRST_POS(alignment, mask, overflow); \
    uint8_t lshift  = NUM_BITS_SECOND_POS(     alignment, mask, overflow); \
    type    bitmask = (type) -1 >> rshift << lshift;                       \
    result          = (type) buffer[ buffer_pos ] & bitmask;               \
    if (overflow)                                                          \
      result = (result << overflow) | (buffer[ buffer_pos + 1 ] >>         \
          (alignment - overflow));                                         \
    else                                                                   \
      result >>= lshift;                                                   \
    printf("[%s:%s:%d]: overflow: %d, lshift: %d, rshift = %d, buffer_pos: %d" \
      ", bitmask: %X\n", __FUNCTION__, __FILE__, __LINE__, \
      overflow, lshift, rshift, buffer_pos, (uint32_t)bitmask);\
  } while(0)

static int64_t rawget(lbuf_t * lbuf, mask_t * mask);

static uint8_t get_alignment(lbuf_t * lbuf, mask_t * mask)
{
  uint8_t alignment = lbuf->alignment;
  if (mask->length <= 8)
    alignment = 8;
  else if (mask->length <= 16)
    alignment = 16;
  else if (mask->length <= 32)
    alignment = 32;
  else if (mask->length <= 64)
    alignment = 64;
  return alignment;
}

// TODO: check limit
static int64_t rawget(lbuf_t * lbuf, mask_t * mask)
{
  uint8_t  alignment       = get_alignment(lbuf, mask);
  uint32_t buffer_pos      = mask->offset / alignment; 
  uint32_t alignment_limit = alignment * (buffer_pos + 1);
  uint32_t last_bit_pos    = mask->offset + mask->length - 1;
  uint8_t  overflow        = last_bit_pos >= alignment_limit ? 
    (last_bit_pos % alignment_limit) + 1 : 0;

  int64_t result = 0;
#ifdef DEBUG
  printf("[%s:%s:%d]: alignment: %d, mask->offset: %d, mask->length: %d\n",
      __FUNCTION__, __FILE__, __LINE__, alignment, mask->offset, mask->length);
#endif
  switch (alignment) {
    case 8:
      APPLY_MASK(uint8_t, lbuf, mask, buffer_pos, overflow, result);
      break;
    case 16:
      APPLY_MASK(uint16_t, lbuf, mask, buffer_pos, overflow, result);
      break;
    case 32:
      APPLY_MASK(uint32_t, lbuf, mask, buffer_pos, overflow, result);
      break;
    case 64:
      APPLY_MASK(uint64_t, lbuf, mask, buffer_pos, overflow, result);
      break;
  }
#ifdef DEBUG
  printf("[%s:%s:%d]: result: %#llx\n",
      __FUNCTION__, __FILE__, __LINE__, (uint64_t) result, result);
#endif
  return result;
}

static int buf_nref_inc(lua_State * L, void * buffer)
{
  lua_Integer c    = 1;
  lbuf_t *    lbuf = (lbuf_t *) luaL_checkudata(L, 1, "lbuf");

  lua_getmetatable(L, 1);
  lua_getfield(L, -1, "__nrefs"); // __nrefs
  lua_pushlightuserdata(L, buffer); // __nrefs[ buffer ]
  lua_gettable(L, -2);
  if (lua_isnumber(L, -1))
    c = lua_tointeger(L, -1) + (lua_Integer) 1;
  else if (!lua_isnil(L, -1)) {
    lua_pop(L, 3); // pops value, field __nrefs, metatable

    return 0;
  }

  lua_pop(L, 1); // pops value
  lua_pushlightuserdata(L, buffer); // __nrefs[ buffer ]
  lua_pushinteger(L, c);
  lua_settable(L, -3);
  lua_pop(L, 2); // pops field __nrefs, metatable
#ifdef DEBUG
  printf("nrefs[ %#x ]: %d\n", buffer, c);
#endif

  return 1;
}

static int buf_nref_dec(lua_State * L, void * buffer)
{
  lbuf_t * lbuf = (lbuf_t *) luaL_checkudata(L, 1, "lbuf");

  lua_getmetatable(L, 1);
  lua_getfield(L, -1, "__nrefs"); // __nrefs
  lua_pushlightuserdata(L, buffer); // __nrefs[ buffer ]
  lua_gettable(L, -2);
  if (!lua_isnumber(L, -1)) {
    lua_pop(L, 3); // pops value, field __nrefs, metatable

    return 0;
  }
  lua_Integer c = lua_tointeger(L, -1);
  lua_pop(L, 1); // pops value
  if (c > 0) {
    c -= (lua_Integer) 1;
    lua_pushlightuserdata(L, buffer); // __nrefs[ buffer ]
    lua_pushinteger(L, c);
    lua_settable(L, -3);
#ifdef DEBUG
    printf("nrefs[ %#x ]: %d\n", buffer, c);
#endif
  }
  lua_pop(L, 2); // pops field __nrefs, metatable
  lua_pushinteger(L, c);

  return 1;
}

// TODO: pass a free function
lbuf_t * lbuf_new(lua_State * L, void * buffer, size_t size)
{
  lbuf_t * lbuf = lua_newuserdata(L, sizeof(lbuf_t));
  lbuf->buffer  = buffer;
  lbuf->size    = size;
  lbuf->alignment = 8;
  luaL_getmetatable(L, "lbuf");
  lua_setmetatable(L, -2);
  buf_nref_inc(L, buffer);
  return lbuf;
}

// TODO: implement lbuf.new(string | table)
static int lbuf_new_(lua_State *L)
{
  lbuf_t * lbuf = lbuf_new(L, buffer, 4);

#ifdef DEBUG
  printf("[%s:%s:%d]: buffer: ", __FUNCTION__, __FILE__, __LINE__);
  for (int i = 0; i < 4; i++) printf("%#x, ", (int) buffer[ i ]);
  printf("\n");
#endif
  return 1;
}

static int lbuf_mask(lua_State *L)
{
  // TODO: create and use isudata()
  lbuf_t * lbuf = (lbuf_t *) luaL_checkudata(L, 1, "lbuf");

  if (lua_isnumber(L, 2)) {
      // TODO: when set align, erase mask table!
    lbuf->alignment = (uint8_t) lua_tointeger(L, 2);
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

    uint8_t length = lua_tointeger(L, -1);
    if (offset + length > lbuf->size * 8) { 
      lua_pop(L, 2);
      break;
    }

    lua_pushvalue(L, -2); // key
    //TODO: check if null
    mask_t * mask = lua_newuserdata(L, sizeof(mask_t)); 
    luaL_getmetatable(L, "lbuf.mask");
    lua_setmetatable(L, -2);

    mask->offset = offset;
    // TODO: also handle tables
    mask->length = length;
    offset       = mask->offset + mask->length;

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
  lbuf_t * lbuf = (lbuf_t *) luaL_checkudata(L, 1, "lbuf");

  // remove entry from masks table (mt.masks[self] = nil)
  lua_getmetatable(L, 1);
  lua_getfield(L, -1, "__masks");
  lua_pushvalue(L, 1);
  lua_pushnil(L);
  lua_settable(L, -3);

  if (!buf_nref_dec(L, lbuf->buffer))
    return 0;

  lua_Integer c = lua_tointeger(L, -1);
  if (c == 0) {
#ifdef DEBUG
    // TODO: free lbuf->buffer
    printf("[%s:%s:%d]: lbuf->buffer must be freed.\n", __FUNCTION__, __FILE__, __LINE__);
#endif
  }

  return 1;
}

static int lbuf_get(lua_State *L)
{
  lbuf_t * lbuf = (lbuf_t *) luaL_checkudata(L, 1, "lbuf");

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
    if (ix < 1 || ix * lbuf->alignment > lbuf->size * 8) { 
      lua_pushnil(L);
      return 1;
    }

    mask_t mask = { (ix - 1) * lbuf->alignment, lbuf->alignment };
    int64_t value = rawget(lbuf, &mask);
    lua_pushnumber(L, value);
    return 1;
  }

  lua_pushvalue(L, 2); // [ ix | __masks[ lbuf ] | (...) ]
  lua_gettable(L, -2); // [ __masks[ lbuf ][ ix ] | ix | __masks[ lbuf ] | (...) ]
  if (lua_isnil(L, -1)) 
    return 1;

  mask_t * mask = (mask_t *) luaL_checkudata(L, -1, "lbuf.mask");

  int64_t value = rawget(lbuf, mask);
  lua_pushnumber(L, value);
  return 1;
}

static int lbuf_rawget(lua_State *L)
{
  lbuf_t * lbuf = (lbuf_t *) luaL_checkudata(L, 1, "lbuf");
  uint8_t ix  = (uint8_t) luaL_checkinteger(L, 2);
  uint8_t len = (uint8_t) luaL_checkinteger(L, 3);

  if (ix < 0 || ix + len > lbuf->size * 8) { 
    lua_pushnil(L);
    return 1;
  }

  mask_t mask = { ix, len };
  int64_t value = rawget(lbuf, &mask);
  lua_pushnumber(L, value);
  return 1;
}

static const luaL_Reg lbuf[ ] = {
  {"new", lbuf_new_},
  {"mask", lbuf_mask},
  {"rawget", lbuf_rawget},
  {NULL, NULL}
};

static const luaL_Reg lbuf_m[ ] = {
  {"mask", lbuf_mask},
  {"rawget", lbuf_rawget},
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
  lua_setfield(L, 2, "__masks");
  lua_newtable(L);
  lua_setfield(L, 2, "__nrefs");
  luaL_register(L, "lbuf", lbuf);

  luaL_newmetatable(L, "lbuf.mask");
  luaL_register(L, NULL, mask_m);
  return 1;
}

