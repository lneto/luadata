luadata
=======

Luadata provides C and Lua APIs to handle binary data using Lua scripts. Here is a briefly description of those:

## Lua API

### creation

#### data.new(table)

Returns a new data object initialized with the given byte array. For example:
```Lua
d1 = data.new{0xFF, 0xFE, 0x00} --> returns a data object with 3 bytes.
```

### layout

#### data.layout(table)

Returns a new layout table based on table argument, which should have the following formats for its fields:

1. field = {\<offset\>, \<length\> \[, \<endian\>\]} or
2. field = {offset = \<offset\>, length = \<length\> \[endian = \<endian\>\]}

Where, field is the name of the field, \<offset\> is the offset in bits (MSB 0), \<length\> is the length in bits, endian is a string that indicates the field endianness ('host', 'net', 'little', 'big'). The default value for endian is 'big'.

Here are a couple examples:

* format 1:
```Lua
l1 = data.layout{
  msb      = {0,  1},
  uint32   = {0, 32},
  uint64le = {0, 64, 'little'}
}
```

* format 2:
```Lua
l2 = data.layout{
  msb                  = {offset = 0, length = 1},
  net_unaligned_uint16 = {offset = 1, length = 16, endian = 'net'}
}
```
* d:layout(layout | table)

Applies a layout table on a given data object. If a regular table is passed, it calls data.layout(table) first. For example:

```Lua
d1:layout(l1) -- applies l1 layout into d1 data object
d2:layout{byte = {0, 8}} -- creates and applies a new layout into d2 data object
```

## C API

### creation

#### int ldata_newref(lua_State *L, void *ptr, size_t size);

Creates a new data object pointing to ptr (without copying it), leaves the data object on the top of the Lua stack and returns a reference
for it. The data object will not be garbage-collected until it is unreferred.

### deletion

#### void ldata_unref(lua_State *L, int ref);

Removes the ptr from the data object and releases the data-object reference, allowing it to be garbage-collected. After that, it is safe
to free the ptr pointer.
