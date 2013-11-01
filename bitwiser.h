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

#ifndef BITWISER_H
#define BITWISER_H

#include <stddef.h>

#include <lua.h>

// TODO: do not expose this struct
typedef struct {
  void *  buffer;
  size_t  size;
  uint8_t alignment;
  bool    net;
  bool    lalloc;
#if 0
  struct {
    uint8_t net : 1;
    uint8_t lua : 1;
  } config;
  uint8_t step;
#endif
  uint8_t step;
} bitwiser_buffer_t;

//typedef struct bitwiser_buffer bitwiser_buffer_t;

bitwiser_buffer_t * bitwiser_newbuffer(lua_State * L, void * buffer, size_t size);

#endif
