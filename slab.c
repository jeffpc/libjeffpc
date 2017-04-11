/*
 * Copyright (c) 2015-2017 Josef 'Jeff' Sipek <jeffpc@josefsipek.net>
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

#include <errno.h>
#include <string.h>

#include <jeffpc/error.h>
#include <jeffpc/mem.h>

/*
 * A slab allocator - well, not really... just use malloc and free directly.
 */

#pragma weak mem_cache_create
struct mem_cache *mem_cache_create(char *name, size_t size, size_t align)
{
	struct mem_cache *cache;

	if (!size || align)
		return ERR_PTR(-EINVAL);

	cache = malloc(sizeof(struct mem_cache));
	if (!cache)
		return ERR_PTR(-ENOMEM);

	cache->size = size;
	cache->align = align;

	return cache;
}

#pragma weak mem_cache_destroy
void mem_cache_destroy(struct mem_cache *cache)
{
	if (!cache)
		return;

	free(cache);
}

#pragma weak mem_cache_alloc
void *mem_cache_alloc(struct mem_cache *cache)
{
	return malloc(cache->size);
}

#pragma weak mem_cache_free
void mem_cache_free(struct mem_cache *cache, void *buf)
{
	free(buf);
}
