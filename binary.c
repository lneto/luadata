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
#ifndef _KERNEL
#include <limits.h>
#include <sys/param.h>
#include <sys/types.h>
#else
#if defined(__NetBSD__)
#include <machine/limits.h>
#include <sys/param.h>
#elif defined(__linux__)
#include <linux/kernel.h>
#endif
#endif

#include <sys/endian.h>

#include "binary.h"

#define BYTE_MAX	UCHAR_MAX
#define UINT64_BIT	(64)

inline static void
set_bits(uint64_t *value, uint64_t clear_mask, uint64_t set_mask)
{
	*value &= clear_mask;
	*value |= set_mask;
}

#define CONTIGUOUS_BITS(widt, truncated)	(width - truncated)

static void
expand(uint64_t *value, size_t width, size_t msb_offset, byte_t truncated)
{
	size_t contiguous = CONTIGUOUS_BITS(width, truncated);

	size_t trunc_msb_offset = BYTE_BIT - truncated;
	size_t trunc_lsb_offset = contiguous + trunc_msb_offset;

	size_t clear_offset = msb_offset + truncated;

	uint64_t clear_mask = UINT64_MAX >> clear_offset;
	uint64_t trunc_mask = *value >> trunc_lsb_offset << contiguous;

	set_bits(value, clear_mask, trunc_mask);
}

static void
contract(uint64_t *value, size_t width, size_t msb_offset, byte_t truncated)
{
	size_t contiguous = CONTIGUOUS_BITS(width, truncated);

	size_t trunc_lsb_offset = BYTE_BIT - truncated;
	size_t trunc_msb_offset = contiguous + trunc_lsb_offset;

	size_t clear_offset = msb_offset + truncated;

	uint64_t clear_mask = UINT64_MAX << clear_offset;
	uint64_t trunc_mask = *value << trunc_msb_offset >> contiguous;

	set_bits(value, clear_mask, trunc_mask);
}

#define TRUNCATED_BITS(width)		(width % BYTE_BIT)

#define VALUE_MSB_OFFSET(width)		(UINT64_BIT - width)

static void
swap_bytes_in(uint64_t *value, size_t width)
{
	size_t msb_offset = VALUE_MSB_OFFSET(width);

	*value <<= msb_offset;

	*value = bswap64(*value);

	byte_t truncated = TRUNCATED_BITS(width);
	if (truncated > 0)
		expand(value, width, msb_offset, truncated);
}

static void
swap_bytes_out(uint64_t *value, size_t width)
{
	size_t msb_offset = VALUE_MSB_OFFSET(width);

	*value = bswap64(*value);

	byte_t truncated = TRUNCATED_BITS(width);
	if (truncated > 0)
		contract(value, width, msb_offset, truncated);

	*value >>= msb_offset;
}

#define MSB_OFFSET(offset)		(offset % BYTE_BIT)

#define LSB_OFFSET(width, msb_offset)					\
	MAX((ssize_t) (BYTE_BIT - msb_offset - width), 0)

#define BYTE_POSITION(offset)		(offset / BYTE_BIT)

#define OVERFLOW_BITS(width, msb_offset, lsb_offset) 			\
	(width - (BYTE_BIT - msb_offset - lsb_offset))

#define MASK(msb_offset, lsb_offset) 					\
	((byte_t) BYTE_MAX >> (msb_offset + lsb_offset) << lsb_offset)

#define OVERFLOW_LSB_OFFSET(overflow)	(BYTE_BIT - overflow)

#define NEED_SWAP(width, endian) 					\
	(width > BYTE_BIT && endian == LITTLE_ENDIAN)

uint64_t
binary_get_uint64(byte_t *bytes, size_t offset, size_t width, int endian)
{
	size_t msb_offset = MSB_OFFSET(offset);
	size_t lsb_offset = LSB_OFFSET(width, msb_offset);
	size_t pos        = BYTE_POSITION(offset);
	size_t overflow   = OVERFLOW_BITS(width, msb_offset, lsb_offset);
	byte_t mask       = MASK(msb_offset, lsb_offset);

	uint64_t value = (byte_t) (bytes[ pos ] & mask) >> lsb_offset;

	for (; overflow >= BYTE_BIT; overflow -= BYTE_BIT)
		value = (value << BYTE_BIT) | bytes[ ++pos ];

	if (overflow > 0) {
		/* assertion: overflow < BYTE_BIT */
		size_t overflow_lsb_offset = OVERFLOW_LSB_OFFSET(overflow);

		value <<= overflow;
		value  |= (byte_t) bytes[ ++pos ] >> overflow_lsb_offset;
	}

	if (NEED_SWAP(width, endian))
		swap_bytes_in(&value, width);

	return value;
}

void
binary_set_uint64(byte_t *bytes, size_t offset, size_t width, int endian,
	uint64_t value)
{
	size_t msb_offset = MSB_OFFSET(offset);
	size_t lsb_offset = LSB_OFFSET(width, msb_offset);
	size_t pos        = BYTE_POSITION(offset);
	size_t overflow   = OVERFLOW_BITS(width, msb_offset, lsb_offset);
	byte_t clear_mask = ~MASK(msb_offset, lsb_offset);

	if (NEED_SWAP(width, endian))
		swap_bytes_out(&value, width);

	bytes[ pos ] &= clear_mask;
	bytes[ pos ] |= (uint64_t) value << lsb_offset >> overflow;

	for (; overflow >= BYTE_BIT; overflow -= BYTE_BIT)
		bytes[ ++pos ] = (uint64_t) value >> (overflow - BYTE_BIT);

	if (overflow > 0) {
		/* assertion: overflow < BYTE_BIT */
		size_t overflow_lsb_offset = OVERFLOW_LSB_OFFSET(overflow);
		byte_t overflow_clear_mask = ~(BYTE_MAX << overflow_lsb_offset);

		bytes[ ++pos ] &= overflow_clear_mask;
		bytes[   pos ] |= value << overflow_lsb_offset;
	}
}
