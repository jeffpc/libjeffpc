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

#include "buffer_impl.h"

static int stdio_buffer_check_truncate(struct buffer *buffer, size_t size)
{
	return -ESPIPE;
}

static ssize_t stdio_buffer_check_seek(struct buffer *buffer, off_t offset,
				       int whence, size_t newoff)
{
	/* allow no-op seeks */
	if (newoff == buffer->used)
		return 0;

	return -ENOTSUP;
}

static void stdio_buffer_copyin(struct buffer *buffer, size_t off,
				const void *newdata, size_t newdatalen)
{
	if (fwrite(newdata, newdatalen, 1, buffer->private) != 1)
		panic("%s: failed to write data to buffer", __func__);
}

const struct buffer_ops stdio_buffer = {
	.check_truncate = stdio_buffer_check_truncate,
	.check_seek = stdio_buffer_check_seek,

	/*
	 * no need for:
	 *  - realloc since we use SIZE_MAX alloc size
	 *  - free since there is nothing to free
	 */

	.clear = generic_buffer_clear_panic,
	.copyin = stdio_buffer_copyin,
};
