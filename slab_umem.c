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
#include <umem.h>

#include <jeffpc/error.h>
#include <jeffpc/mem.h>

/*
 * A slab allocator - wrapping libumem.
 */

struct mem_cache *mem_cache_create(char *name, size_t size, size_t align)
{
	struct mem_cache *cache;

	cache = (struct mem_cache *) umem_cache_create(name, size, align,
						       NULL, NULL, NULL,
						       NULL, NULL, 0);
	if (!cache)
		return ERR_PTR(-errno);

	return cache;
}

void mem_cache_destroy(struct mem_cache *cache)
{
	umem_cache_destroy((umem_cache_t *) cache);
}

void *mem_cache_alloc(struct mem_cache *cache)
{
	return umem_cache_alloc((umem_cache_t *) cache, 0);
}

void mem_cache_free(struct mem_cache *cache, void *buf)
{
	umem_cache_free((umem_cache_t *) cache, buf);
}
