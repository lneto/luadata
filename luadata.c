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
#ifndef _KERNEL
#include <string.h>
#else
#if defined(__NetBSD__)
#include <lib/libkern/libkern.h>
#elif defined(__linux__)
#include <linux/kernel.h>
#endif
#endif

#include <lua.h>
#include <lauxlib.h>

#include "luautil.h"

#include "luadata.h"
#include "data.h"
#include "layout.h"

static char *
new_data_num(lua_State *L, size_t *len)
{
	*len = luau_tosize(L, 1);
	if (*len == 0)
		return NULL;

	char *data = (char *) luau_malloc(L, *len);
	if (data == NULL)
		return NULL;

	memset(data, 0, *len);
	return data;
}

static char *
new_data_tab(lua_State *L, size_t *len)
{
#if LUA_VERSION_NUM >= 502
	*len = lua_rawlen(L, 1);
#else
	*len = lua_objlen(L, 1);
#endif
	if (*len == 0)
		return NULL;

	char *data = (char *) luau_malloc(L, *len);
	if (data == NULL)
		return NULL;

	size_t i = 0;
	lua_pushnil(L);  /* first key */
	while (lua_next(L, 1) != 0 && i < *len) {
		/* uses 'key' (at index -2) and 'value' (at index -1) */
		data[ i++ ] = (char) lua_tointeger(L, -1);
		/* removes 'value'; keeps 'key' for next iteration */
		lua_pop(L, 1);
	}
	return data;
}

static char *
new_data_str(lua_State *L, size_t *len)
{
	const char *str = lua_tolstring(L, 1, len);
	if (str == NULL || *len == 0)
		return NULL;

	char *data = (char *) luau_malloc(L, *len);
	if (data == NULL)
		return NULL;

	memcpy(data, str, *len);
	return data;
}

static int
new_data(lua_State *L)
{
	char   *data = NULL;
	size_t len   = 0;

	int type = lua_type(L, 1);
	if (type == LUA_TNUMBER)
		data = new_data_num(L, &len);
	else if (type == LUA_TTABLE)
		data = new_data_tab(L, &len);
	else if (type == LUA_TSTRING)
		data = new_data_str(L, &len);

	if (data == NULL || len == 0)
		return 0;

	data_new(L, (void *) data, len, true);
	return 1;
}

static int
new_layout(lua_State *L)
{
	if (!lua_istable(L, 1))
		return 0;

	layout_load(L, 1);
	return 1;
}

static int
new_segment(lua_State *L)
{
	data_t *data = lua_touserdata(L, 1);

	size_t offset = data->offset;
	size_t length = data->length;

	int nargs = lua_gettop(L);
	if (nargs >= 2) {
		size_t seg_offset = luau_tosize(L, 2);

		offset += seg_offset;

		if (nargs >= 3)
			length = luau_tosize(L, 3);
		else
			length -= seg_offset;
	}

	return data_new_segment(L, data, offset, length);
}

static int
apply_layout(lua_State *L)
{
	data_t *data = lua_touserdata(L, 1);

	if (!lua_istable(L, 2))
		return 0;

	lua_getfield(L, 2, LAYOUT_STAMP);
	bool is_layout = (bool) lua_toboolean(L, -1);
	lua_pop(L, 1);

	if (!is_layout)
		layout_load(L, 2);

	data_apply_layout(L, data, 2);

	/* return data object */
	lua_pushvalue(L, 1);
	return 1;
}

static int
__gc(lua_State *L)
{
	data_t *data = lua_touserdata(L, 1);
	data_delete(L, data);
	return 0;
}

static int
__index(lua_State *L)
{
	data_t *data = lua_touserdata(L, 1);

	/* try object-oriented access first */
	luau_getmetatable(L, 1, 2);
	if (!lua_isnil(L, -1))
		/* return this method */
		return 1;

	lua_pop(L, 1);
	return data_get_field(L, data, 2);
}

static int
__newindex(lua_State *L)
{
	data_t *data = lua_touserdata(L, 1);

	/* try object-oriented access first */
	luau_getmetatable(L, 1, 2);
	if (!lua_isnil(L, -1))
		/* shouldn't overwrite a method */
		goto end;

	data_set_field(L, data, 2, 3);
end:
	lua_pop(L, 1);
	return 0;
}

inline static int
__len(lua_State *L)
{
	data_t *data = lua_touserdata(L, 1);

	luau_pushsize(L, data->length);
	return 1;
}

inline static int
__tostring(lua_State *L)
{
	data_t *data = lua_touserdata(L, 1);

	const char * ptr = (const char *) data_get_ptr(data);
	lua_pushlstring(L, ptr, data->length);
	return 1;
}

static const luaL_Reg data_lib[ ] = {
	{"new"   , new_data},
	{"layout", new_layout},
	{NULL    , NULL}
};

static const luaL_Reg data_m[ ] = {
	{"layout"    , apply_layout},
	{"segment"   , new_segment},
	{"__index"   , __index},
	{"__newindex", __newindex},
	{"__gc"      , __gc},
	{"__len"     , __len},
	{"__tostring", __tostring},
	{NULL        , NULL}
};

static const luaL_Reg layout_entry_m[ ] = {
	{NULL, NULL}
};

int
luaopen_data(lua_State *L)
{
	luaL_newmetatable(L, LAYOUT_ENTRY_USERDATA);
#if LUA_VERSION_NUM >= 502
	luaL_setfuncs(L, layout_entry_m, 0);
#else
	luaL_register(L, NULL, layout_entry_m);
#endif
	lua_pop(L, 1);

	luaL_newmetatable(L, DATA_USERDATA);
#if LUA_VERSION_NUM >= 502
	luaL_setfuncs(L, data_m, 0);
	luaL_newlib(L, data_lib);
#else
	luaL_register(L, NULL, data_m);
	luaL_register(L, DATA_LIB, data_lib);
#endif

	return 1;
}

int
ldata_newref(lua_State *L, void *ptr, size_t size)
{
	data_new(L, ptr, size, false);
	/* keep the new data object on the stack */
	lua_pushvalue(L, -1);
	return luau_ref(L);
}

#if defined(_KERNEL) && defined(__NetBSD__)
int
ldata_newref_chain(lua_State *L, struct mbuf *chain)
{
	data_new_chain(L, chain, false);
	/* keep the new data object on the stack */
	lua_pushvalue(L, -1);
	return luau_ref(L);
}
#endif

void
ldata_unref(lua_State *L, int r)
{
	luau_getref(L, r);

	data_t *data = data_test(L, -1);
	if (data == NULL)
		return;

	data_unref(data);

	/* pop data object */
	lua_pop(L, 1);
	luau_unref(L, r);
}

void *
ldata_topointer(lua_State *L, int index, size_t *size)
{
	data_t *data = data_test(L, index);
	if (data == NULL) {
		if (size != NULL)
			*size = 0;
		return NULL;
	}

	if (size != NULL)
		*size = data->length;

	return data_get_ptr(data);
}

#if defined(_KERNEL) && defined(_MODULE)
#if defined(__NetBSD__) 
#include <sys/lua.h>
#include <sys/module.h>

MODULE(MODULE_CLASS_MISC, luadata, "lua");

static int
luadata_modcmd(modcmd_t cmd, void *opaque)
{
	int error;

	switch (cmd) {
	case MODULE_CMD_INIT:
		error = klua_mod_register(DATA_LIB, luaopen_data);
		break;
	case MODULE_CMD_FINI:
		error = klua_mod_unregister(DATA_LIB);
		break;
	default:
		error = ENOTTY;
	}
	return error;
}
#elif defined(__linux__)
#include <linux/module.h>

EXPORT_SYMBOL(luaopen_data);

static int __init data_init(void)
{
        return 0;
}

static void __exit data_exit(void)
{
}

module_init(data_init);
module_exit(data_exit);
#endif
#endif
