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
#ifndef _BINARY_H_
#define _BINARY_H_

#include <sys/endian.h>

#ifndef _KERNEL
#include <stddef.h>
#include <stdint.h>
#else
#if defined(__NetBSD__)
#include <sys/types.h>
#elif defined(__linux__)
#include <linux/types.h>
#endif
#endif

#define BYTE_BIT	CHAR_BIT

typedef unsigned char byte_t;

uint64_t binary_get_uint64(byte_t *, size_t, size_t, int);

void binary_set_uint64(byte_t *, size_t, size_t, int, uint64_t);

#define CEIL_DIV(x, y)	((x + y - 1) / y)
#define BIT_TO_BYTE(x)	(CEIL_DIV(x, BYTE_BIT))
#define BYTE_TO_BIT(x)	(x * BYTE_BIT)

#endif /* _BINARY_H_ */
