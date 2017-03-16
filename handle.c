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
#ifdef _KERNEL
#if defined(__NetBSD__)
#include <machine/limits.h>
#elif defined(__linux__)
#include <linux/kernel.h>
#endif
#endif

#include <lauxlib.h>

#include "luautil.h"

#include "handle.h"

static void
free_handle(lua_State *L, handle_t *handle)
{
	switch (handle->type) {
	case HANDLE_TYPE_SINGLE:
	{
		single_t *single = &handle->bucket.single;
		luau_free(L, single->ptr, single->size);
		break;
	}
	case HANDLE_TYPE_CHAIN:
	{
#if defined(_KERNEL) && defined(__NetBSD__)
		struct mbuf *chain = handle->bucket.chain;
		m_free(chain);
#endif
		break;
	}
	}
}

handle_t *
handle_new_single(lua_State *L, void *ptr, size_t size, bool free)
{
	handle_t *handle = (handle_t *) luau_malloc(L, sizeof(handle_t));

	single_t *single = &handle->bucket.single;
	single->ptr  = ptr;
	single->size = size;

	handle->type     = HANDLE_TYPE_SINGLE;
	handle->refcount = 0;
	handle->free     = free;

	return handle;
}

#if defined(_KERNEL) && defined(__NetBSD__)
handle_t *
handle_new_chain(lua_State *L, struct mbuf *chain, bool free)
{
	handle_t *handle = (handle_t *) luau_malloc(L, sizeof(handle_t));

	handle->bucket.chain = chain;

	handle->type     = HANDLE_TYPE_CHAIN;
	handle->refcount = 0;
	handle->free     = free;

	return handle;
}
#endif

void
handle_delete(lua_State *L, handle_t *handle)
{
	if (handle->refcount == 0) {
		if (handle->free)
			free_handle(L, handle);

		luau_free(L, handle, sizeof(handle_t));
	}
	else
		handle->refcount--;
}

void *
handle_get_ptr(handle_t *handle, size_t offset, size_t length)
{
	void *ptr = NULL;

	switch (handle->type) {
	case HANDLE_TYPE_SINGLE:
	{
		single_t *single = &handle->bucket.single;
		if (single->ptr == NULL)
			return NULL;

		ptr = (char *) single->ptr + offset;
		break;
	}
	case HANDLE_TYPE_CHAIN:
	{
#if defined(_KERNEL) && defined(__NetBSD__)
		struct mbuf *chain = handle->bucket.chain;
		if (chain == NULL || offset > INT_MAX || length > INT_MAX)
			return NULL;

		struct mbuf *m = m_pulldown(chain, (int) offset, (int) length,
				NULL);
		if (m == NULL)
			return NULL;

		ptr = mtod(m, void *);
#endif
	}
	}

	return ptr;
}

void
handle_unref(handle_t *handle)
{
	switch (handle->type) {
	case HANDLE_TYPE_SINGLE:
	{
		single_t *single = &handle->bucket.single;
		single->ptr  = NULL;
		single->size = 0;
		break;
	}
	case HANDLE_TYPE_CHAIN:
	{
#if defined(_KERNEL) && defined(__NetBSD__)
		handle->bucket.chain = NULL;
#endif
		break;
	}
	}
}

