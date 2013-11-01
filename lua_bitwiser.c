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

#include <lauxlib.h>

#include "lua_bitwiser.h"
#include "bitwiser.h"
#include "lua_util.h"
#include "bit_util.h"

#define DEBUG

#define CHECK_LIMITS(buffer, offset, length)        \
  (offset < 0 || offset + length > buffer->size * 8) 

#define CHECK_MASKED_LIMITS(buffer, mask)          \
  CHECK_LIMITS(buffer, mask->offset, mask->length)


typedef struct {
// TODO: use two ix levels (bit and align (e.g., byte))?
  uint32_t offset;
  uint8_t  length;
  // TODO: add modifiers here, such as net, endian, sign, step
} mask_t;

/* static prototypes */
static int get_masked_field(lua_State * L, bitwiser_buffer_t * buffer);
  
static int set_masked_field(lua_State * L, bitwiser_buffer_t * buffer, int64_t value);

static bool load_mask_from_table(lua_State *L, mask_t * mask);

static bool load_mask(lua_State *L, mask_t * mask);

static void copy_mask(lua_State *L, mask_t * mask);

static void load_mask_table(lua_State *L, int index);

/* exported functions */
int lbw_new_buffer(lua_State *L)
{
  if(!lua_istable(L, 1))
    return 0;
// TODO: implement bitwiser.buffer(<string>)

  size_t len = lua_objlen(L, 1);
  printf("len: %u\n", (unsigned) len);
  int8_t * buffer_ = (int8_t *) lua_malloc(L, len);

  int i = 0;
  lua_pushnil(L);  /* first key */
  while (lua_next(L, 1) != 0 && i < len) {
    /* uses 'key' (at index -2) and 'value' (at index -1) */
    buffer_[ i ] = (int8_t) lua_tointeger(L, -1); 
    i++;
    /* removes 'value'; keeps 'key' for next iteration */
    lua_pop(L, 1);
  }
  //printf("created buffer (b[0] = %d\n", buffer_[0]);
#if 0
  for (i = 0, lua_pushnil(L); i < len; i++, lua_next(L, 1)) {
    /* uses 'key' (at index -2) and 'value' (at index -1) */
    buffer_[ i ] = lua_tointeger(L, -1); 
    /* removes 'value'; keeps 'key' for next iteration */
    lua_pop(L, 1);
  }
#endif
  bitwiser_buffer_t * buffer = bitwiser_newbuffer(L, (void *) buffer_, len);
  buffer->lalloc = true;

#ifdef DEBUG
  printf("[%s:%s:%d]: buffer: ", __FUNCTION__, __FILE__, __LINE__);
  for (int i = 0; i < len; i++) printf("%#x, ", (int) buffer_[ i ]);
  printf("\n");
#endif
  return 1;
}

int lbw_new_mask(lua_State *L)
{
  if (!lua_istable(L, 1)) 
    return 0;
  load_mask_table(L, 1);
  return 1;
}

int lbw_apply_mask(lua_State *L)
{
  bitwiser_buffer_t * buffer = (bitwiser_buffer_t *) 
    luaL_checkudata(L, 1, "bitwiser.buffer");

  if (!lua_istable(L, 2))
    return 0;

  // TODO: we need to create a new userdata (mask_table)
  lua_getfield(L, 2, "__mask_table");
  bool is_mask_table = (bool) lua_toboolean(L, -1);
  lua_pop(L, 1);

  if (!is_mask_table) 
    load_mask_table(L, 2);

  lua_getmetatable(L, 1);
  lua_getfield(L, -1, "__masks");
  lua_pushvalue(L, 1); // push buffer userdatum
  lua_pushvalue(L, 2); // push mask_table
  lua_settable(L, -3); // __masks[ buffer ] = mask_table

  // TODO: define what we should return
  lua_pushvalue(L, 1); // return buffer userdata
  return 1;
}

int lbw_gc(lua_State *L)
{
  // TODO: remove entry from masks table! (mt.masks[self] = nil)
  // TODO: free bitwiser->buffer if ref count == 0
  bitwiser_buffer_t * buffer = (bitwiser_buffer_t *)
    luaL_checkudata(L, 1, "bitwiser.buffer");

  if (buffer->lalloc)
    lua_free(L, buffer->buffer, buffer->size);
  return 0;
}

int lbw_get_field(lua_State *L)
{
  // TODO: put this on a macro
  bitwiser_buffer_t * buffer = (bitwiser_buffer_t *) 
    luaL_checkudata(L, 1, "bitwiser.buffer");

  lua_getmetatable(L, 1);
  
  // try object-oriented access
  if (lua_isstring(L, 2)) {
    lua_pushvalue(L, 2);
    lua_gettable(L, -2);
    if (lua_iscfunction(L, -1))
      return 1; // return this method
    else
      lua_pop(L, 1);
  }
  return get_masked_field(L, buffer);
}

int lbw_set_field(lua_State *L)
{
  bitwiser_buffer_t * buffer = (bitwiser_buffer_t *) 
    luaL_checkudata(L, 1, "bitwiser.buffer");
  int64_t value = (int64_t) lua_tointeger(L, 3);

  // TODO: verify if some method will be rewritten!

  lua_getmetatable(L, 1);
  return set_masked_field(L, buffer, value);
}

int lbw_get(lua_State *L)
{
  bitwiser_buffer_t * buffer = (bitwiser_buffer_t *) 
    luaL_checkudata(L, 1, "bitwiser.buffer");
  uint32_t offset = (uint32_t) luaL_checkinteger(L, 2);
  uint8_t  length = (uint8_t)  luaL_checkinteger(L, 3);

// TODO: create a CHECK macro
  if (offset < 0 || offset + length > buffer->size * 8) { 
    lua_pushnil(L);
    return 1;
  }

  int64_t value = get_bits(buffer->buffer, offset, length, buffer->net);
  lua_pushnumber(L, value);
  return 1;
}

// TODO: implement lbw_set()

/* static functions */
// TODO: how to implement this?
#if 0
mask_t * get_mask(lua_State *L, bitwiser_buffer_t * buffer)
{
  lua_getfield(L, -1, "__masks"); // get hidden __masks table
  lua_pushvalue(L, 1); // push buffer userdatum
  lua_gettable(L, -2); // get __masks[ buffer ]
  if (!lua_istable(L, -1)) {
  // TODO: buffer:mask(alignment) is not supported anymore; must implement
  // mask{ __step = ...}
#if 0
    uint32_t ix = (uint32_t) luaL_checkinteger(L, 2);
    printf("ix: %d\n", ix);
    if (ix < 1 || ix * buffer->step > buffer->size * 8) { 
      lua_pushnil(L);
      return 1;
    }
    // TODO: create get_offset macro
    uint32_t offset = (ix - 1) * buffer->step;
    int64_t  value = get_bits(buffer->buffer, offset, buffer->step, buffer->net);
    lua_pushnumber(L, value);
    return 1;
#endif
    return 0;
  }

// TODO: put this chunk in a function of macro
  lua_pushvalue(L, 2); // push key
  lua_gettable(L, -2); // [ __masks[ buffer ][ key ] 
  if (lua_isnil(L, -1)) 
    // TODO: should return none?
    return 1;

  mask_t * mask = (mask_t *) luaL_checkudata(L, -1, "bitwiser.mask");

  if (CHECK_MASKED_LIMITS(buffer, mask)) 
    return 0;
}
#endif

static int get_masked_field(lua_State * L, bitwiser_buffer_t * buffer)
{
  lua_getfield(L, -1, "__masks"); // get hidden __masks table
  lua_pushvalue(L, 1); // push buffer userdatum
  lua_gettable(L, -2); // get __masks[ buffer ]
  if (!lua_istable(L, -1)) {
  // TODO: buffer:mask(alignment) is not supported anymore; must implement
  // mask{ __step = ...}
#if 0
    uint32_t ix = (uint32_t) luaL_checkinteger(L, 2);
    printf("ix: %d\n", ix);
    if (ix < 1 || ix * buffer->step > buffer->size * 8) { 
      lua_pushnil(L);
      return 1;
    }
    // TODO: create get_offset macro
    uint32_t offset = (ix - 1) * buffer->step;
    int64_t  value = get_bits(buffer->buffer, offset, buffer->step, buffer->net);
    lua_pushnumber(L, value);
    return 1;
#endif
    return 0;
  }

// TODO: put this chunk in a function of macro
  lua_pushvalue(L, 2); // push key
  lua_gettable(L, -2); // [ __masks[ buffer ][ key ] 
  if (lua_isnil(L, -1)) 
    // TODO: should return none?
    return 1;

  mask_t * mask = (mask_t *) luaL_checkudata(L, -1, "bitwiser.mask");

  if (CHECK_MASKED_LIMITS(buffer, mask)) 
    return 0;

  int64_t value = get_bits(buffer->buffer, mask->offset, mask->length, buffer->net);
  lua_pushnumber(L, value);
  return 1;
}

static int set_masked_field(lua_State * L, bitwiser_buffer_t * buffer, int64_t value)
{
  lua_getfield(L, -1, "__masks");
  lua_pushvalue(L, 1); // [ lubf | __masks | getmetatable(bitwiser) | ix | bitwiser ]
  lua_gettable(L, -2); // [ __masks[ bitwiser ] | bitwiser | __masks | (...) ]
  if (!lua_istable(L, -1)) {
  // TODO: buffer:mask(alignment) is not supported anymore; must implement
  // mask{ __step = ...}
#if 0
    uint8_t ix = (uint8_t) luaL_checkinteger(L, 2);
    if (ix < 1 || ix * bitwiser->alignment > bitwiser->size * 8) { 
      return 0;
    }

    uint32_t offset = (ix - 1) * bitwiser->alignment;
    set_bits(bitwiser->buffer, offset, bitwiser->alignment, bitwiser->net, value);
#endif
    return 0;
  }

  lua_pushvalue(L, 2); // push key
  lua_gettable(L, -2); // [ __masks[ buffer ][ key ] 
  if (lua_isnil(L, -1)) 
    // TODO: should return none?
    return 1;

  mask_t * mask = (mask_t *) luaL_checkudata(L, -1, "bitwiser.mask");

  set_bits(buffer->buffer, mask->offset, mask->length, buffer->net, value);
  return 0;
}

static bool load_mask_from_table(lua_State *L, mask_t * mask)
{
  size_t table_len = lua_objlen(L, -1);

  // TODO: [lneto] handle __signed and __endian fields!
  // TODO: put these two structures on functions or macros!
  if (table_len >= 1) {
    lua_getnfield(L, -1, 1);
    if (!lua_isnumber(L, -1)) 
      return false;
    mask->offset = lua_tointeger(L, -1); 
    lua_pop(L, 1);
  }
  if (table_len >= 2) {
    lua_getnfield(L, -1, 2);
    if (!lua_isnumber(L, -1)) 
      return false;
    mask->length = lua_tointeger(L, -1); 
    lua_pop(L, 1);
  }

  lua_getfield(L, -1, "__offset");
  if (lua_isnumber(L, -1))
    mask->offset = lua_tointeger(L, -1);
  lua_pop(L, 1);

  lua_getfield(L, -1, "__length");
  if (lua_isnumber(L, -1))
    mask->length = lua_tointeger(L, -1);
  lua_pop(L, 1);
  return true;
}

static bool load_mask(lua_State *L, mask_t * mask)
{
// TODO: implement support for {'name', len} fields
    if (lua_isnumber(L, -1)) { // length
      mask->length  = lua_tointeger(L, -1); 
      mask->offset += mask->length;
    }
    else if (lua_istable(L, -1)) // { offset, len }
      if (!load_mask_from_table(L, mask))
        return false;
    return true;
}

static void copy_mask(lua_State *L, mask_t * mask)
{
  //TODO: check if null (is it necessary?)
  mask_t * new_mask = lua_newuserdata(L, sizeof(mask_t)); 
  luaL_getmetatable(L, "bitwiser.mask");
  lua_setmetatable(L, -2);

  new_mask->offset = mask->offset;
  new_mask->length = mask->length;
}

static void load_mask_table(lua_State *L, int index)
{
  mask_t mask = { .offset = 0, .length = 0 };
  // TODO: create a new clean table to put masks!
  lua_pushnil(L);  /* first key */
  while (lua_next(L, index) != 0) {
    /* uses 'key' (at index -2) and 'value' (at index -1) */
    if (!load_mask(L, &mask))
      lua_pushnil(L); // mask_table[ix] = nil (remove this item)

    lua_pushvalue(L, -2); // push key (to set mask ud to this field)
    copy_mask(L, &mask);

    lua_settable(L, index); // mask_table[ key ] = mask
    /* removes 'value'; keeps 'key' for next iteration */
    lua_pop(L, 1);
  }
  // TODO: this code is provisory; we need to make a new userdata!
  lua_pushboolean(L, true);
  lua_setfield(L, index, "__mask_table");
}

/* end of module */
