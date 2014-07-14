#include <stdlib.h>
#include <stdio.h>
#include <stddef.h>
#include <assert.h>

#include <lua.h>
#include <lauxlib.h>

#include "luadata.h"

typedef unsigned char byte_t;

int
main(void)
{
	/* create a new Lua state */
	lua_State *L = luaL_newstate();

	/* open luadata library */
#if LUA_VERSION_NUM >= 502
	luaL_requiref(L, "data", luaopen_data, 1);
#else
	luaopen_data(L);
#endif
	lua_pop(L, 1);  /* remove lib */

	/* load a script */
	assert(luaL_dofile(L, "ctest.lua") == 0);
	
	/* get the filter function */
	lua_getglobal(L, "filter");

	/* create a new data ptr */
	size_t  data_size = 3;
	byte_t *data_ptr  = (byte_t *) malloc(data_size);
	data_ptr[0] = 0xAB;
	data_ptr[1] = 0xCD;
	data_ptr[2] = 0xEF;

	/* create a new data object */
	int rd = ldata_newref(L, data_ptr, data_size);

	/* call data_filter(d)*/
	assert(lua_pcall(L, 1, 1, 0) == 0);

	/* get filter result */
	int passed = lua_toboolean(L, -1);
	assert(passed);

	/* unregister the Lua data object */
	ldata_unref(L, rd);

	/* now we can safely free the data pointer */
	free(data_ptr);
	data_ptr  = NULL;
	data_size = 0;

	/* get the access_global function */
	lua_getglobal(L, "access_global");

	/* call access_global() to try to access a data with a freed ptr */
	assert(lua_pcall(L, 0, 1, 0) == 0);

	/* should return nil */
	assert(lua_isnil(L, -1));

	/* get a pointer of data object created by the Lua script */
	lua_getglobal(L, "d");
	data_ptr = (byte_t *) ldata_topointer(L, -1, &data_size);
	assert(data_ptr != NULL);

	/* check size and values */
	assert(data_size == 4);
	assert(data_ptr[0] == 0xFF);
	assert(data_ptr[1] == 0xEE);
	assert(data_ptr[2] == 0xDD);
	assert(data_ptr[3] == 0x00);
	lua_pop(L, 1);

	/* remove data object refered by 'd' from Lua */
	lua_pushnil(L);
	lua_setglobal(L, "d");
	lua_gc(L, LUA_GCCOLLECT, 0);

	lua_getglobal(L, "d");
	data_ptr = (byte_t *) ldata_topointer(L, -1, &data_size);
	assert(data_ptr == NULL);
	assert(data_size == 0);

	printf("test passed ;-)\n");
	return 0;
}

