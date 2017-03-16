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
#ifndef _LAYOUT_H_
#define _LAYOUT_H_

#ifndef _KERNEL
#include <stdint.h>
#include <stdbool.h>
#else
#if defined(__NetBSD__)
#include <sys/types.h>
#endif
#endif

#include <sys/endian.h>

#include <lua.h>

#define LAYOUT_ENTRY_USERDATA 	"data.layout.entry"

#define LAYOUT_STAMP 		"__layout_stamp"

#define LAYOUT_TYPE_DEFAULT	LAYOUT_TNUMBER
#define LAYOUT_ENDIAN_DEFAULT	BIG_ENDIAN

typedef enum {
	LAYOUT_TNUMBER = 0,
	LAYOUT_TSTRING
} layout_type_t;

typedef struct {
	size_t        offset;
	size_t        length;
	layout_type_t type;
	int           endian;
} layout_entry_t;

void layout_load(lua_State *, int);

layout_entry_t * layout_get_entry(lua_State *, int, int);

#endif /* _LAYOUT_H_ */
