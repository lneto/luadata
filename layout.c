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
#include <lua.h>
#include <lauxlib.h>

#include "luautil.h"

#include "layout.h"

inline static layout_entry_t *
test_entry(lua_State *L, int index)
{
#if LUA_VERSION_NUM >= 502
	return (layout_entry_t *)
		luaL_testudata(L, index, LAYOUT_ENTRY_USERDATA);
#else
	return (layout_entry_t *)
		luaL_checkudata(L, index, LAYOUT_ENTRY_USERDATA);
#endif
}

inline static void
init_layout(layout_entry_t *entry)
{
	entry->offset = 0;
	entry->length = 0;
	entry->type   = LAYOUT_TYPE_DEFAULT;
	entry->endian = LAYOUT_ENDIAN_DEFAULT;
}

static void
load_type(lua_State *L, layout_entry_t *entry)
{
	const char *type = lua_tostring(L, -1);

	if (type[0] == 'n')
		entry->type = LAYOUT_TNUMBER;
	else if (type[0] == 's')
		entry->type = LAYOUT_TSTRING;
}

static void
load_endian(lua_State *L, layout_entry_t *entry)
{
	const char *endian = lua_tostring(L, -1);

	if (endian[0] == 'n' || endian[0] == 'b')
		entry->endian = BIG_ENDIAN;
	else if (endian[0] == 'l')
		entry->endian = LITTLE_ENDIAN;
	else if (endian[0] == 'h')
		entry->endian = BYTE_ORDER;
}

static void
load_entry_numbered(lua_State *L, layout_entry_t *entry)
{
#if LUA_VERSION_NUM >= 502
	size_t array_len = lua_rawlen(L, -1);
#else
	size_t array_len = lua_objlen(L, -1);
#endif

	if (array_len >= 1)
		entry->offset = luau_getarray_integer(L, -1, 1);
	if (array_len >= 2)
		entry->length = luau_getarray_integer(L, -1, 2);
	if (array_len >= 3) {
		luau_getarray(L, -1, 3);
		load_type(L, entry);
		lua_pop(L, 1);
	}
	if (array_len >= 4) {
		luau_getarray(L, -1, 4);
		load_endian(L, entry);
		lua_pop(L, 1);
	}
}

static void
load_entry_named(lua_State *L, layout_entry_t *entry)
{
	lua_getfield(L, -1, "offset");
	if (lua_isnumber(L, -1))
		entry->offset = lua_tointeger(L, -1);
	lua_pop(L, 1);

	lua_getfield(L, -1, "length");
	if (lua_isnumber(L, -1))
		entry->length = lua_tointeger(L, -1);
	lua_pop(L, 1);

	lua_getfield(L, -1, "type");
	if (lua_isstring(L, -1))
		load_type(L, entry);
	lua_pop(L, 1);

	lua_getfield(L, -1, "endian");
	if (lua_isstring(L, -1))
		load_endian(L, entry);
	lua_pop(L, 1);
}

static void
load_entry(lua_State *L, layout_entry_t *entry)
{
	init_layout(entry);
	load_entry_numbered(L, entry);
	load_entry_named(L, entry);
}

static void
copy_entry(layout_entry_t *dst, layout_entry_t *src)
{
	dst->offset = src->offset;
	dst->length = src->length;
	dst->type   = src->type;
	dst->endian = src->endian;
}

static int
new_entry(lua_State *L, layout_entry_t *entry)
{
	layout_entry_t *nentry =
		(layout_entry_t *) lua_newuserdata(L, sizeof(layout_entry_t));

	luau_setmetatable(L, LAYOUT_ENTRY_USERDATA);

	copy_entry(nentry, entry);
	return 1;
}

void
layout_load(lua_State *L, int index)
{
	layout_entry_t entry;

	lua_pushnil(L);  /* first key */
	while (lua_next(L, index) != 0) {
		/* uses 'key' (at index -2) and 'value' (at index -1) */
		load_entry(L, &entry);
		if (entry.length == 0)
			lua_pushnil(L);
		else
			new_entry(L, &entry);

		/* layout[ key ] = entry */
		luau_settable(L, index, -3, -1);

		/* removes 'value' and 'entry'; keeps 'key' for next iteration */
		lua_pop(L, 2);
	}

	lua_pushboolean(L, true);
	lua_setfield(L, index, LAYOUT_STAMP);
}

layout_entry_t *
layout_get_entry(lua_State *L, int layout_ix, int key_ix)
{
	void *entry = NULL;

	luau_getref(L, layout_ix);
	luau_gettable(L, -1, key_ix);
	if (lua_isnil(L, -1))
		goto end;

	entry = test_entry(L, -1);

end:
	lua_pop(L, 2);
	return entry;
}

