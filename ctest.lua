function filter(d)
	-- store d in a global
	gd = d
	d:layout{
		-- most significant 4-bytes
		uint4_msb = {0,4},
		-- inner 16-bytes
		uint16    = {4, 16},
		-- least significant 4-bytes
		uint4_lsb = {20, 4}
	}
	return	d.uint4_msb == 0xa    and
		d.uint16    == 0xbcde and
		d.uint4_lsb == 0xf
end

function access_global()
	-- should return nil if ldata_unref(d) has been called from C
	return gd.uint16
end

d = data.new{0xff, 0xee, 0xdd, 0x00}
