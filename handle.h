/*
 * Copyright (c) 2014, Lourival Vieira Neto <lneto@NetBSD.org>.
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
#ifndef _HANDLE_H_
#define _HANDLE_H_

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

typedef struct {
	void  *ptr;
	size_t size;
} single_t;

typedef union {
	single_t single;
#if defined(_KERNEL) && defined(__NetBSD__)
	struct mbuf *chain;
#endif
} bucket_t;

typedef enum {
	HANDLE_TYPE_SINGLE = 0,
	HANDLE_TYPE_CHAIN,
} handle_type_t;

typedef struct {
	bucket_t      bucket;
	handle_type_t type;
	size_t        refcount;
	bool          free;
} handle_t;

handle_t * handle_new_single(lua_State *, void *, size_t, bool);

#if defined(_KERNEL) && defined(__NetBSD__)
handle_t * handle_new_chain(lua_State *, struct mbuf *, bool);
#endif

void handle_delete(lua_State *, handle_t *);

void * handle_get_ptr(handle_t *, size_t, size_t);

void handle_unref(handle_t *);

#endif /* _HANDLE_H_ */
