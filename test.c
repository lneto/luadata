#include <stdlib.h>
#include <stdio.h>

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
	luaopen_data(L);

	/* load a script */
	if (luaL_dofile(L, "ctest.lua") != 0)
		goto err;
	
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
	if (lua_pcall(L, 1, 1, 0) != 0)
		goto err;

	/* get filter result */
	bool passed = (bool) lua_toboolean(L, -1);

	/* unregister the Lua data object */
	ldata_unref(L, rd);

	/* now we can safely free the data pointer */
	free(data_ptr);

	/* get the access_global function */
	lua_getglobal(L, "access_global");

	/* call access_global() to try to access a data with a freed ptr */
	if (lua_pcall(L, 0, 1, 0) != 0)
		goto err;

	/* if access_global() returned nil and filter() has passed; then test passed */
	if (lua_isnil(L, -1) && passed)
		printf("test passed ;-)\n");

	return 0;
err:
	return 1;
}

