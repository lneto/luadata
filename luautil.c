/*
 * Copyright (c) 2013, 2014, Lourival Vieira Neto <lneto@NetBSD.org>.
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
#include "luautil.h"

inline static void
adjust_index(int *index, int factor)
{
	if (*index < 0)
		(*index) -= factor;
}

void
luau_getarray(lua_State *L, int index, lua_Integer n)
{
	lua_pushinteger(L, n);
	adjust_index(&index, 1);
	lua_gettable(L, index);
}

lua_Integer
luau_getarray_integer(lua_State *L, int index, lua_Integer n)
{
	luau_getarray(L, index, n);
	lua_Integer result = lua_tointeger(L, -1);
	lua_pop(L, 1);
	return result;
}

void
luau_gettable(lua_State *L, int index, int field_index)
{
	lua_pushvalue(L, field_index);
	adjust_index(&index, 1);
	lua_gettable(L, index);
}

void
luau_settable(lua_State *L, int index, int field_index, int value_index)
{
	lua_pushvalue(L, field_index);
	adjust_index(&value_index, 1);
	lua_pushvalue(L, value_index);
	adjust_index(&index, 2);
	lua_settable(L, index);
}

void
luau_getmetatable(lua_State *L, int index, int field_index)
{
	lua_getmetatable(L, index);
	adjust_index(&field_index, 1);
	luau_gettable(L, -1, field_index);
	/* pop metatable */
	lua_remove(L, -2);
}

inline void
luau_setmetatable(lua_State *L, const char *tname)
{
	luaL_getmetatable(L, tname);
	lua_setmetatable(L, -2);
}

void *
luau_malloc(lua_State *L, size_t size)
{
	void * ud = NULL;
	lua_Alloc alloc = lua_getallocf(L, &ud);
	return alloc(ud, NULL, 0, size);
}

void
luau_free(lua_State *L, void * ptr, size_t size)
{
	void * ud = NULL;
	lua_Alloc alloc = lua_getallocf(L, &ud);
	alloc(ud, ptr, size, 0);
}

