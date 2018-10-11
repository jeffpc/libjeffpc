/*
 * Copyright (c) 2017-2018 Josef 'Jeff' Sipek <jeffpc@josefsipek.net>
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

#include "buffer_impl.h"

static int static_buffer_check_append(struct buffer *buffer, const void *data,
				      size_t size)
{
	if ((buffer->used + size) <= buffer->allocsize)
		return 0;

	return -ENOSPC;
}

static int static_buffer_check_truncate(struct buffer *buffer, size_t size)
{
	if (size <= buffer->allocsize)
		return 0;

	return -ENOSPC;
}

const struct buffer_ops static_buffer = {
	.check_append = static_buffer_check_append,
	.check_truncate = static_buffer_check_truncate,

	/*
	 * no need for:
	 *  - realloc since we have a borrowed static buffer
	 *  - free since we have a borrowed static buffer
	 */

	.clear = generic_buffer_clear_memset,
	.copyin = generic_buffer_copyin_memcpy,
};
