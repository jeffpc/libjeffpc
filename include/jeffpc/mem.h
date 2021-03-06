/*
 * Copyright (c) 2015-2020 Josef 'Jeff' Sipek <jeffpc@josefsipek.net>
 *
 * Permission is hereby granted, free of charge, to any person obtaining a copy
 * of this software and associated documentation files (the "Software"), to deal
 * in the Software without restriction, including without limitation the rights
 * to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
 * copies of the Software, and to permit persons to whom the Software is
 * furnished to do so, subject to the following conditions:
 *
 * The above copyright notice and this permission notice shall be included in
 * all copies or substantial portions of the Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
 * IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
 * FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
 * AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
 * LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
 * OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
 * SOFTWARE.
 */

#ifndef __JEFFPC_MEM_H
#define __JEFFPC_MEM_H

#include <stdlib.h>
#include <stdint.h>
#include <string.h>

#include <jeffpc/config.h>

#ifdef JEFFPC_HAVE_ALLOCA_H
#include <alloca.h>
#endif

/*
 * zeroed allocation
 */

#define zalloc(len)	mem_zalloc(len)
static inline void *mem_zalloc(size_t len)
{
	void *buf;

	buf = malloc(len);
	if (buf)
		memset(buf, 0, len);

	return buf;
}

static inline void mem_freezero(void *buf, size_t len)
{
	if (!buf)
		return;

	memset(buf, 0, len);

	free(buf);
}

/*
 * array allocator
 */
extern void *mem_reallocarray(void *ptr, size_t nelem, size_t size);
extern void *mem_recallocarray(void *ptr, size_t old_nelem, size_t new_nelem,
			       size_t size);

/*
 * slab allocator
 */

struct mem_cache;

extern struct mem_cache *mem_cache_create(char *name, size_t size, size_t align);
extern void mem_cache_destroy(struct mem_cache *cache);

extern void *mem_cache_alloc(struct mem_cache *cache);
extern void mem_cache_free(struct mem_cache *cache, void *buf);

#endif
