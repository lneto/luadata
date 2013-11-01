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

#include "bitwiser.h"
#include "lua_bitwiser.h"

#define DEBUG
// TODO: create DEBUG(...) macro!

// TODO:  - clean up!
//        - review int types!
//        - gc
//        - improve limits and other verifications

/* C API section */

// TODO: pass a free function
bitwiser_buffer_t * bitwiser_newbuffer(lua_State * L, void * buffer, size_t size)
{
  bitwiser_buffer_t * bitbuffer = lua_newuserdata(L, sizeof(bitwiser_buffer_t));

  bitbuffer->buffer = buffer;
  bitbuffer->size   = size;
  bitbuffer->step   = 8;
  bitbuffer->net    = true;
  bitbuffer->lalloc = false;

  luaL_getmetatable(L, "bitwiser.buffer");
  lua_setmetatable(L, -2);
  return bitbuffer;
}

static const luaL_Reg bitwiser_lib[ ] = {
  {"buffer" , lbw_new_buffer},
  {"mask"   , lbw_new_mask},
  {NULL     , NULL}
};

static const luaL_Reg buffer_m[ ] = {
  {"mask"      , lbw_apply_mask},
  {"get"       , lbw_get},
#if 0
  {"set"       , set},
  {"segment"   , segment},
#endif
  {"__index"   , lbw_get_field},
  {"__newindex", lbw_set_field},
  {"__gc"      , lbw_gc},
  {NULL        , NULL}
};

static const luaL_Reg mask_m[ ] = {
  {NULL, NULL}
};

int luaopen_bitwiser(lua_State *L)
{
  luaL_newmetatable(L, "bitwiser.buffer");
  luaL_register(L, NULL, buffer_m);
  lua_newtable(L);
  lua_setfield(L, -2, "__masks");
  luaL_register(L, "bitwiser", bitwiser_lib);

  luaL_newmetatable(L, "bitwiser.mask");
  luaL_register(L, NULL, mask_m);
  return 1;
}
/* end of module */
