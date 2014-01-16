require "data"

-- create a new data object
d1 = data.new{0x0f}

-- create and apply a data layout
d1:layout{byte = {0, 8}, lsb = {7, 1}}

-- access the whole byte 
assert(d1.byte == 0x0f)

-- set 0 to the least significant bit
d1.lsb = 0

-- access the whole byte again
assert(d1.byte == 0x0e)

-- create a new layout
l = data.layout{
	uint16be = {0, 16},
	uint16le = {0, 16, 'l'},
	uint4    = {16, 4},
	uint9    = {20, 9},
	overflow = {32, 1},
}

-- create a new data object
d2 = data.new{0xaa, 0xbb, 0xcc, 0xdd}

-- apply the layout 'l' to data 'd2'
d2:layout(l)

-- access 2 bytes using big-endian ordering
assert(d2.uint16be == 0xaabb)

-- access 2 bytes using little-endian ordering
assert(d2.uint16le == 0xbbaa)

-- access 4 bits 
assert(d2.uint4 == 0xc)

-- access out of bounds
assert(d2.overflow == nil)

print("test passed ;-)")
