/*
 * Copyright (c) 2013, 2014 Lourival Vieira Neto <lneto@NetBSD.org>.
 * All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 * 1. Redistributions of source code must retain the above copyright
 *    notice, this list of conditions and the following disclaimer.
 * 2. Redistributions in binary form must reproduce the above copyright
 *    notice, this list of conditions and the following disclaimer in the
 *    documentation and/or other materials provided with the distribution.
 * 3. The name of the Author may not be used to endorse or promote products
 *    derived from this software without specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY THE AUTHOR AND CONTRIBUTORS ``AS IS'' AND
 * ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
 * IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
 * ARE DISCLAIMED.  IN NO EVENT SHALL THE REGENTS OR CONTRIBUTORS BE LIABLE
 * FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL
 * DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS
 * OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION)
 * HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT
 * LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY
 * OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF
 * SUCH DAMAGE.
 */
#ifndef _LUA_UTIL_H_
#define _LUA_UTIL_H_

#ifndef _KERNEL
#include <stddef.h>
#endif

#include <lua.h>
#include <lauxlib.h>

#define luau_isvalidref(ref)	(ref != LUA_REFNIL && ref != LUA_NOREF)

#define luau_ref(L)		luaL_ref(L, LUA_REGISTRYINDEX)
#define luau_unref(L, r)	luaL_unref(L, LUA_REGISTRYINDEX, r)
#define luau_getref(L, r)	lua_rawgeti(L, LUA_REGISTRYINDEX, r)

#define luau_tosize(L, index)	((size_t) lua_tointeger(L, index))
#define luau_pushsize(L, size)	lua_pushinteger(L, (lua_Integer) size)

void luau_getarray(lua_State *, int, lua_Integer);

lua_Integer luau_getarray_integer(lua_State *, int, lua_Integer);

void luau_gettable(lua_State *, int, int);

void luau_settable(lua_State *, int, int, int);

void luau_getmetatable(lua_State *, int, int);

void * luau_malloc(lua_State *, size_t);

void luau_free(lua_State *, void *, size_t);

#endif /* _LUA_UTIL_H_ */
