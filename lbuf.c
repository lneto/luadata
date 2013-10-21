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
#include <stdbool.h>

#include <lua.h>
#include <lauxlib.h>

#include <arpa/inet.h>

#include "bit_util.h"

#define DEBUG
// TODO: create DEBUG(...) macro!

// TODO:  - clean up!
//        - review int types!
//        - gc
//        - improve limits and other verifications
// NEXT_VERSION:
//        - named fields
//        - implement set and rawset

uint8_t buffer[ 4 ] = { 0xFF, 0x00, 0xFF, 0x00 };

typedef void (* free_t) (void * ud, void * ptr);

typedef struct {
  uint8_t alignment;
  void *  buffer;
  size_t  size;
  bool    net;
  free_t  free;
  void *  free_ud;
} lbuf_t;

typedef struct {
// TODO: use two ix levels (bit and align (e.g., byte))?
  uint32_t offset;
  uint8_t  length;
} mask_t;

// XXX: [review] I would rename this to inc_nref or inc_buffer_nref
// (I think buf prefix is a little confusing)
static int buf_nref_inc(lua_State * L, void * buffer)
{
// XXX: [review] rename c to a more expressive name, such as ref_count or nref.
// XXX: [review] it should be init with 0, instead of 1. However, I would just
// do 'lua_Integer nref += lua_tointeger(L, -1)' below.
  lua_Integer c    = 1;
// XXX: [review] don't need to check it again, put a pre cond instead.
  lbuf_t *    lbuf = (lbuf_t *) luaL_checkudata(L, 1, "lbuf");

// TODO: [lneto] create a get_nref function with this code
  lua_getmetatable(L, 1);
  lua_getfield(L, -1, "__nrefs"); // __nrefs
  lua_pushlightuserdata(L, buffer); // __nrefs[ buffer ]
  lua_gettable(L, -2);
// XXX: [review] I think that is safe to simply call lua_tointeger() (without
// checking if it is number). If you don't think so, you can return -1 if fail.
  if (lua_isnumber(L, -1))
    c = lua_tointeger(L, -1) + (lua_Integer) 1;
// XXX: I think it is unecessary.
  else if (!lua_isnil(L, -1)) {
    lua_pop(L, 3); // pops value, field __nrefs, metatable

// XXX: [review] return nref directly (instead using Lua stack).
    return 0;
  }

  lua_pop(L, 1); // pops value
// TODO: [lneto] create a set_nref function with this code
  lua_pushlightuserdata(L, buffer); // __nrefs[ buffer ]
  lua_pushinteger(L, c);
  lua_settable(L, -3);
  lua_pop(L, 2); // pops field __nrefs, metatable
#ifdef DEBUG
  printf("nrefs[ %p ]: %td\n", buffer, c);
#endif

// XXX: [review] return nref directly (instead using Lua stack).
  return 1;
}

// XXX: [review] I would rename this to dec_nref or dec_buffer_nref
// (I think buf prefix is a little confusing)
static int buf_nref_dec(lua_State * L, void * buffer)
{
// XXX: [review] don't need to check it again, put a pre cond instead.
  lbuf_t * lbuf = (lbuf_t *) luaL_checkudata(L, 1, "lbuf");

// TODO: [lneto] create a get_nref function with this code
  lua_getmetatable(L, 1);
  lua_getfield(L, -1, "__nrefs"); // __nrefs
  lua_pushlightuserdata(L, buffer); // __nrefs[ buffer ]
  lua_gettable(L, -2);
// XXX: [review] I think that is safe to simply call lua_tointeger() (without
// checking if it is number). If you don't think so, you can return -1 if fail.
  if (!lua_isnumber(L, -1)) {
    lua_pop(L, 3); // pops value, field __nrefs, metatable

// XXX: [review] return nref directly (instead using Lua stack).
    return 0;
  }
  lua_Integer c = lua_tointeger(L, -1);
  lua_pop(L, 1); // pops value
  if (c > 0) {
// XXX: [review] c-- would be just fine.
    c -= (lua_Integer) 1;
// TODO: [lneto] create a set_nref function with this code
    lua_pushlightuserdata(L, buffer); // __nrefs[ buffer ]
    lua_pushinteger(L, c);
    lua_settable(L, -3);
#ifdef DEBUG
    printf("nrefs[ %p ]: %td\n", buffer, c);
#endif
  }
  lua_pop(L, 2); // pops field __nrefs, metatable
  lua_pushinteger(L, c);

// XXX: [review] return nref directly (instead using Lua stack).
  return 1;
}

// TODO: pass a free function
// XXX: [review] should receive 'void * free_ud' too
lbuf_t * lbuf_new(lua_State * L, void * buffer, size_t size, free_t free)
{
  lbuf_t * lbuf = lua_newuserdata(L, sizeof(lbuf_t));
  lbuf->buffer  = buffer;
  lbuf->size    = size;
  lbuf->alignment = 8;
  lbuf->net = true;
  lbuf->free    = free;
// XXX: [review] set ud here.
  lbuf->free_ud = NULL;
  luaL_getmetatable(L, "lbuf");
  lua_setmetatable(L, -2);
  buf_nref_inc(L, buffer);
  return lbuf;
}

// TODO: implement lbuf.new(string | table)
static int lbuf_new_(lua_State *L)
{
  lbuf_t * lbuf = lbuf_new(L, buffer, 4, NULL);

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

    uint8_t length = lua_tonumber(L, -1);
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

// XXX: [review] get nref directly (instead using Lua stack).
  if (!buf_nref_dec(L, lbuf->buffer))
// XXX: [review] shouldn't return here.
    return 0;

// XXX: [review] rename c to a more expressive name, such as ref_count or nref.
  lua_Integer c = lua_tointeger(L, -1);

  if (c == 0 && lbuf->free != NULL)
    lbuf->free(lbuf->free_ud, lbuf->buffer);

// XXX: [review] should return 0 here.
  return 1;
}

static int lbuf_get(lua_State *L)
{
  lbuf_t * lbuf = (lbuf_t *) luaL_checkudata(L, 1, "lbuf");

  lua_getmetatable(L, 1);
  
  // access to methods (such as mask())
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

    // TODO: create get_offset macro
    uint32_t offset = (ix - 1) * lbuf->alignment;
    int64_t  value = get_bits(lbuf->buffer, offset, lbuf->alignment, lbuf->net);
    lua_pushnumber(L, value);
    return 1;
  }

  lua_pushvalue(L, 2); // [ ix | __masks[ lbuf ] | (...) ]
  lua_gettable(L, -2); // [ __masks[ lbuf ][ ix ] | ix | __masks[ lbuf ] | (...) ]
  if (lua_isnil(L, -1)) 
    return 1;

  mask_t * mask = (mask_t *) luaL_checkudata(L, -1, "lbuf.mask");

  int64_t value = get_bits(lbuf->buffer, mask->offset, mask->length, lbuf->net);
  lua_pushnumber(L, value);
  return 1;
}

static int lbuf_set(lua_State *L)
{
  lbuf_t * lbuf = (lbuf_t *) luaL_checkudata(L, 1, "lbuf");
  int64_t value = (int64_t) lua_tointeger(L, 3);

  lua_getmetatable(L, 1);

  lua_getfield(L, -1, "__masks");
  lua_pushvalue(L, 1); // [ lubf | __masks | getmetatable(lbuf) | ix | lbuf ]
  lua_gettable(L, -2); // [ __masks[ lbuf ] | lbuf | __masks | (...) ]
  if (!lua_istable(L, -1)) {
    uint8_t ix = (uint8_t) luaL_checkinteger(L, 2);
    if (ix < 1 || ix * lbuf->alignment > lbuf->size * 8) { 
      return 0;
    }

    uint32_t offset = (ix - 1) * lbuf->alignment;
    set_bits(lbuf->buffer, offset, lbuf->alignment, lbuf->net, value);
    return 0;
  }

  lua_pushvalue(L, 2); // [ ix | __masks[ lbuf ] | (...) ]
  lua_gettable(L, -2); // [ __masks[ lbuf ][ ix ] | ix | __masks[ lbuf ] | (...) ]
  if (lua_isnil(L, -1)) 
    return 0;

  mask_t * mask = (mask_t *) luaL_checkudata(L, -1, "lbuf.mask");

  set_bits(lbuf->buffer, mask->offset, mask->length, lbuf->net, value);
  return 0;
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

  int64_t value = get_bits(lbuf->buffer, ix, len, lbuf->net);
  lua_pushnumber(L, value);
  return 1;
}

// TODO: implement rawset!

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
  {"__newindex", lbuf_set},
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
// XXX: [review] change them back to relative indices
  lua_setfield(L, 2, "__masks");
  lua_newtable(L);
  lua_setfield(L, 2, "__nrefs");
  luaL_register(L, "lbuf", lbuf);

  luaL_newmetatable(L, "lbuf.mask");
  luaL_register(L, NULL, mask_m);
  return 1;
}

