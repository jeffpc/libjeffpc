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

#include <stdbool.h>

#include <jeffpc/int.h>

#include "buffer_impl.h"

struct buffer *buffer_alloc(size_t expected_size)
{
	struct buffer *buffer;

	buffer = malloc(sizeof(struct buffer));
	if (!buffer)
		return ERR_PTR(-ENOMEM);

	buffer->data = malloc(expected_size ? expected_size : 1);
	if (!buffer->data) {
		free(buffer);
		return ERR_PTR(-ENOMEM);
	}

	buffer->off = 0;
	buffer->used = 0;
	buffer->allocsize = expected_size;
	buffer->ops = &heap_buffer;

	return buffer;
}

void buffer_free(struct buffer *buffer)
{
	if (!buffer)
		return;

	if (buffer->ops->free)
		buffer->ops->free(buffer->data);

	free(buffer);
}

void buffer_init_sink(struct buffer *buffer)
{
	buffer->data = NULL;
	buffer->off = 0;
	buffer->used = 0;
	buffer->allocsize = SIZE_MAX;
	buffer->ops = &sink_buffer;
}

void buffer_init_const(struct buffer *buffer, const void *data, size_t size)
{
	buffer->data = (void *) data;
	buffer->off = 0;
	buffer->used = size;
	buffer->allocsize = size;
	buffer->ops = &const_buffer;
}

static int resize(struct buffer *buffer, size_t newsize)
{
	void *tmp;

	if (newsize <= buffer->allocsize)
		return 0;

	if (!buffer->ops->realloc)
		return -ENOTSUP;

	tmp = buffer->ops->realloc(buffer->data, newsize);
	if (!tmp)
		return -ENOMEM;

	buffer->data = tmp;
	buffer->allocsize = newsize;

	return 0;
}

int buffer_append(struct buffer *buffer, const void *data, size_t size)
{
	int ret;

	if (!buffer)
		return -EINVAL;

	if (!data && size)
		return -EINVAL;

	if (buffer->ops->check_append) {
		ret = buffer->ops->check_append(buffer, data, size);
		if (ret)
			return ret;
	}

	if (!size)
		return 0; /* append(..., 0) is a no-op */

	ret = resize(buffer, buffer->used + size);
	if (ret)
		return ret;

	buffer->ops->copyin(buffer, buffer->used, data, size);

	buffer->used += size;

	return 0;
}

ssize_t buffer_seek(struct buffer *buffer, off_t offset, int whence)
{
	size_t newoff;

	if (!buffer)
		return -EINVAL;

	switch (whence) {
		case SEEK_SET:
			if (offset < 0)
				return -EINVAL;
			if (offset > buffer->used)
				return -EINVAL;

			newoff = offset;
			break;
		case SEEK_CUR:
			if ((offset > 0) &&
			    (offset > buffer->used - buffer->off))
				return -EINVAL;
			if ((offset < 0) && (-offset > buffer->off))
				return -EINVAL;

			newoff = buffer->off + offset;
			break;
		case SEEK_END:
			if (offset > 0)
				return -EINVAL;
			if (-offset > buffer->used)
				return -EINVAL;

			newoff = buffer->off + offset;
			break;
		default:
			return -EINVAL;
	}

	if (newoff > SSIZE_MAX)
		return -EOVERFLOW;

	buffer->off = newoff;

	return newoff;
}

int buffer_truncate(struct buffer *buffer, size_t size)
{
	int ret;

	if (!buffer)
		return -EINVAL;

	if (buffer->ops->check_truncate) {
		ret = buffer->ops->check_truncate(buffer, size);
		if (ret)
			return ret;
	}

	ret = resize(buffer, size);
	if (ret)
		return ret;

	if (buffer->used < size)
		buffer->ops->clear(buffer, buffer->used, size - buffer->used);

	buffer->used = size;

	return 0;
}

/*
 * Generic implementations
 */

/* clear implementations */
void generic_buffer_clear_memset(struct buffer *buffer, size_t off, size_t len)
{
	memset(buffer->data + off, 0, len);
}

void generic_buffer_clear_nop(struct buffer *buffer, size_t off, size_t len)
{
}

void generic_buffer_clear_panic(struct buffer *buffer, size_t off, size_t len)
{
	panic("buffer clear called");
}

/* copyin implementations */
void generic_buffer_copyin_memcpy(struct buffer *buffer, size_t off,
				  const void *newdata, size_t newdatalen)
{
	memcpy(buffer->data + off, newdata, newdatalen);
}

void generic_buffer_copyin_nop(struct buffer *buffer, size_t off,
			       const void *newdata, size_t newdatalen)
{
}

void generic_buffer_copyin_panic(struct buffer *buffer, size_t off,
				 const void *newdata, size_t newdatalen)
{
	panic("buffer copyin called");
}
