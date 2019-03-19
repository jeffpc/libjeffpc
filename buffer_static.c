/*
 * Copyright (c) 2017-2019 Josef 'Jeff' Sipek <jeffpc@josefsipek.net>
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

/*
 * Static buffers
 *
 * Since we have a borrowed buffer we must never allow:
 *
 *  - resizing of the buffer
 *  - freeing of the buffer
 *
 * Conveniently, if we leave the realloc op NULL, the generic code returns
 * -ENOTSUP for us.  Leaving the free op NULL turns it into a no-op.
 *
 * There is a slight difference between the read-only and read-write
 * variants.  Namely, the read-only variant does not support *any* writes or
 * changes to the (apparent) buffer size.  In both cases, -EROFS is
 * returned.  The read-write variant allows changes to the amount of used
 * space, truncation, etc. as long as the result fits within the borrowed
 * buffer.
 */

static int static_buffer_check_append_ro(struct buffer *buffer, const void *data,
					 size_t size)
{
	return -EROFS;
}

static int static_buffer_check_append_rw(struct buffer *buffer, const void *data,
					 size_t size)
{
	if ((buffer->size + size) <= buffer->allocsize)
		return 0;

	return -ENOSPC;
}

static int static_buffer_check_truncate_ro(struct buffer *buffer, size_t size)
{
	return -EROFS;
}

static int static_buffer_check_truncate_rw(struct buffer *buffer, size_t size)
{
	if (size <= buffer->allocsize)
		return 0;

	return -ENOSPC;
}

static int static_buffer_check_write_ro(struct buffer *buffer, const void *buf,
				        size_t len, size_t off)
{
	return -EROFS;
}

static int static_buffer_check_write_rw(struct buffer *buffer, const void *buf,
				        size_t len, size_t off)
{
	if ((off + len) <= buffer->allocsize)
		return 0;

	return -ENOSPC;
}

/* a read-only static buffer */
const struct buffer_ops static_buffer_ro = {
	.check_append = static_buffer_check_append_ro,
	.check_truncate = static_buffer_check_truncate_ro,
	.check_write = static_buffer_check_write_ro,

	.clear = generic_buffer_clear_panic,
	.copyin = generic_buffer_copyin_panic,
	.copyout = generic_buffer_copyout_memcpy,
};

/* a read-write static buffer */
const struct buffer_ops static_buffer_rw = {
	.check_append = static_buffer_check_append_rw,
	.check_truncate = static_buffer_check_truncate_rw,
	.check_write = static_buffer_check_write_rw,

	.clear = generic_buffer_clear_memset,
	.copyin = generic_buffer_copyin_memcpy,
	.copyout = generic_buffer_copyout_memcpy,
};
