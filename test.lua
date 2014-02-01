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

-- create a new data segment, where offset = 8, length = 20
d3 = d2:segment(8, 20)

-- apply the layout 'l' into data 'd3'
d3:layout(l)

-- access the first 16 bits of 'd3' segment 
assert(d3.uint16be == 0xbbcc)

-- create a new data segment, where offset = 4, length is unbounded
d4 = d3:segment(4)

-- apply the layout 'l' into data 'd4'
d4:layout(l)

-- access the first 16 bits of 'd4' segment 
assert(d4.uint16be == 0xbccd)

-- create a new data segment comprehending the whole original data
d5 = d2:segment()

-- apply the layout 'l' into data 'd5'
d5:layout(l)

-- set 0xf into 'uint4' position
d5.uint4 = 0xf

-- check that it was set
assert(d5.uint4 == 0xf)

-- check that d5 points to the same raw data of d2 
assert(d2.uint4 == 0xf)

-- force the collecting of d2, d3, d4
d2 = nil
d3 = nil
d4 = nil
collectgarbage()

-- check that d5 is still valid
assert(d5.uint4 == 0xf)

print("test passed ;-)")
