/*
 * Copyright (c) 2017 Josef 'Jeff' Sipek <jeffpc@josefsipek.net>
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

#ifndef __JEFFPC_ARRAY_H
#define __JEFFPC_ARRAY_H

#include <jeffpc/types.h>

extern void *array_alloc(size_t elem_size, size_t prealloc_count);
extern void array_free(void *raw);

extern int array_truncate(void **raw, size_t idx);
extern size_t array_size(void *raw);

/*
 * This silly wrapper lets us have a void ** argument, yet users can pass in
 * a struct foo **.
 */
#define array_truncate(raw, idx)				\
	({							\
		void *_tmp = *(raw);				\
		int ret;					\
		ret = array_truncate(&_tmp, (idx));		\
		*(raw) = _tmp;					\
		ret;						\
	})

#endif
