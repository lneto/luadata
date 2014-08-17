local data = require'data'

-- create a new data object
d1 = data.new{0x0f}

-- with no layout applied, __index and __newindex should return nil
d1.no = 0xdead
assert(d1.no == nil)

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
	uint16le = {0, 16, 'number', 'le'},
	uint4    = {16, 4},
	uint9    = {20, 9},
	overflow = {32, 1},
}

-- create a new data object
d2 = data.new{0xaa, 0xbb, 0xcc, 0xdd}

-- check byte length of d2
assert(#d2 == 4)

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

-- create a new data segment, where offset = 1, length = 3
d3 = d2:segment(1, 3)

-- check bit length of d3
assert(#d3 == 3)

-- apply the layout 'l' into data 'd3'
d3:layout(l)

-- access the first 16 bits of 'd3' segment 
assert(d3.uint16be == 0xbbcc)

-- create a new data segment, where offset = 1, length is unbounded
d4 = d3:segment(1)

-- apply the layout 'l' into data 'd4'
d4:layout(l)

-- access the first 16 bits of 'd4' segment 
assert(d4.uint16be == 0xccdd)

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

-- create a new data object with 2 bytes 
d = data.new(2)
d:layout(l)
d.uint16be = 0xBEEF
assert(d.uint16be == 0xBEEF)

d = data.new(128)
l = data.layout{
        toobig = {0, 65},
        uint64 = {0, 64},
} 
        
d:layout(l)
d.uint64 = -1
assert(d.toobig == nil)
assert(d.uint64 == -1)

-- create a new data object from a string
d = data.new'\a'
d:layout{ascii = {1, 7}} 
assert(d.ascii == 7)

-- create a string field in a layout
d6 = data.new("abcdef")
d6:layout{str = {0, 6, 's'}, overflow = {1, 6, 's'}}

assert(d6.str == "abcdef")
assert(d6.overflow == nil)

d6.str = "hij"
assert(d6.str == "hijdef")

-- check invalid data creation 
d = data.new()
assert(d == nil)

d = data.new(0)
assert(d == nil)

d = data.new{}
assert(d == nil)

d = data.new('')
assert(d == nil)

print("test passed ;-)")
