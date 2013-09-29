#include <stdio.h>
#include <stdint.h>

#include <lua.h>
#include <lauxlib.h>

uint8_t buffer[ 4 ] = { 1 , 2, 4, 8 };

uint8_t get(uint8_t * buffer, size_t n, uint8_t ix, uint8_t align)
{
  printf("[ ix = %d, align = %d]\n", ix, align);
  uint8_t bit_ix  = (ix - 1) * align;
  uint8_t byte_ix = bit_ix / 8;

  uint8_t bit_end  = bit_ix + align - 1;
  uint8_t bit_upper_limit = 8 * (byte_ix + 1);
  uint8_t overflow = bit_end >= bit_upper_limit ? (bit_end % bit_upper_limit) + 1 : 0;
  uint8_t left_shift = !overflow ? (8 - (bit_ix % 8) - align) : 0;
  uint8_t mask     = ((uint8_t) -1 >> (8 - (align - overflow))) << left_shift;

  uint8_t result = (uint8_t) buffer[ byte_ix ] & mask;
  printf("buffer   = %d\n", (int) buffer[ byte_ix ]);
  printf("result   = %d\n", (int) result);

  if (overflow)
    result = (result << overflow) | (buffer[ byte_ix + 1 ] >> (8 - overflow));
  else
    result = result >> left_shift;

  printf("bit_ix   = %d\n", (int) bit_ix);
  printf("bit_end  = %d\n", (int) bit_end);
  printf("byte_ix  = %d\n", (int) byte_ix);
  printf("overflow = %d\n", (int) overflow);
  printf("mask [1] = %d\n", (int) (8 - (align - overflow)));
  printf("mask [2] = %d\n", (int) left_shift);
  printf("mask     = %X\n", (int) mask);
  printf("result   = %d\n", (int) result);
  printf("----------------------\n\n");
  return result;
}

static int lbuf_new(lua_State *L)
{
  lua_pushlightuserdata(L, buffer);
  return 1;
}

static int lbuf_get(lua_State *L)
{
  uint8_t * buffer = (uint8_t *) lua_touserdata(L, 1);

  uint8_t ix = (uint8_t) luaL_checkinteger(L, 2);
  //uint8_t align = (uint8_t) luaL_checkinteger(L, 2);
  uint8_t value = get(buffer, 4, ix, 3);
  lua_pushnumber(L, value);
  return 1;
}

static int lbuf_rawget(lua_State *L)
{
  uint8_t ix    = (uint8_t) luaL_checkinteger(L, 1);
  uint8_t align = (uint8_t) luaL_checkinteger(L, 2);
  uint8_t value = get(buffer, 4, ix, align);
  lua_pushnumber(L, value);
  return 1;
}

#if 0
int main(void)
{
  // args

  //for (size_t i = 0; i < sizeof(buffer); i++)
    printf("%X\n", *((uint32_t *) buffer));

  get(buffer, 4, 1, 3);
  get(buffer, 4, 2, 3);
  get(buffer, 4, 3, 3);
  get(buffer, 4, 4, 3);
  get(buffer, 4, 5, 3);
  get(buffer, 4, 6, 3);
  get(buffer, 4, 7, 3);
  get(buffer, 4, 8, 3);
  get(buffer, 4, 9, 3);
  get(buffer, 4, 10, 3);
  get(buffer, 4, 1, 5);
  get(buffer, 4, 2, 5);
  get(buffer, 4, 3, 5);
  get(buffer, 4, 1, 9);
  get(buffer, 4, 2, 9);
}
#endif

static const luaL_Reg lbuf[ ] = {
  {"new", lbuf_new},
  {"get", lbuf_get},
  {"rawget", lbuf_rawget},
  {NULL, NULL}
};

int luaopen_lbuf(lua_State *L)
{
  luaL_register(L, "lbuf", lbuf);
}

