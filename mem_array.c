/*
 * Copyright (c) 2018 Josef 'Jeff' Sipek <jeffpc@josefsipek.net>
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

#include <jeffpc/mem.h>
#include <jeffpc/config.h>

void *mem_reallocarray(void *ptr, size_t nelem, size_t size)
{
#ifdef HAVE_REALLOCARRAY
	return reallocarray(ptr, nelem, size);
#else
	/* TODO: check for overflow, return NULL & set errno to ENOMEM */
	return realloc(ptr, nelem * size);
#endif
}

void *mem_recallocarray(void *ptr, size_t old_nelem, size_t new_nelem,
			size_t size)
{
#ifdef HAVE_RECALLOCARRAY
	return recallocarray(ptr, old_nelem, new_nelem, size);
#else
	/*
	 * TODO: check for overflow, return NULL & set errno to:
	 *   ENOMEM if new size overflows
	 *   EINVAL if old size overflows
	 */
	const size_t old_size = size * old_nelem;
	const size_t new_size = size * new_nelem;
	char *tmp;

	tmp = mem_reallocarray(ptr, new_nelem, size);

	if (tmp && (new_nelem > old_nelem))
		memset(tmp + old_size, 0, new_size - old_size);

	return tmp;
#endif
}
