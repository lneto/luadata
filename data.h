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
#ifndef _DATA_H_
#define _DATA_H_

#ifndef _KERNEL
#include <stddef.h>
#include <stdbool.h>
#else
#if defined(__NetBSD__)
#include <sys/types.h>
#include <sys/mbuf.h>
#endif
#endif

#include <lua.h>

#include "handle.h"
#include "layout.h"

#define DATA_LIB	"data"
#define DATA_USERDATA	"data.data"

typedef struct {
	handle_t *handle;
	size_t    offset;
	size_t    length;
	int       layout;
} data_t;

data_t * data_new(lua_State *, void *, size_t, bool);

#if defined(_KERNEL) && defined(__NetBSD__)
data_t * data_new_chain(lua_State *, struct mbuf *, bool);
#endif

int data_new_segment(lua_State *, data_t *, size_t, size_t);

void data_delete(lua_State *, data_t *);

data_t * data_test(lua_State *, int);

void data_apply_layout(lua_State *, data_t *, int);

int data_get_field(lua_State *, data_t *, int);

void data_set_field(lua_State *, data_t *, int, int);

void * data_get_ptr(data_t *);

void data_unref(data_t *);

#endif /* _DATA_H_ */
