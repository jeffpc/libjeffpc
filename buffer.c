/*
 * Copyright (c) 2017-2020 Josef 'Jeff' Sipek <jeffpc@josefsipek.net>
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
	int ret;

	buffer = malloc(sizeof(struct buffer));
	if (!buffer)
		return ERR_PTR(-ENOMEM);

	ret = buffer_init_heap(buffer, expected_size);
	if (ret) {
		free(buffer);
		return ERR_PTR(ret);
	}

	buffer->heap = true;

	return buffer;
}

void buffer_free(struct buffer *buffer)
{
	if (!buffer)
		return;

	if (buffer->ops->free)
		buffer->ops->free(buffer->data);

	if (buffer->heap)
		free(buffer);
}

int buffer_init_heap(struct buffer *buffer, size_t expected_size)
{
	buffer->data = malloc(expected_size ? expected_size : 1);
	if (!buffer->data)
		return -ENOMEM;

	buffer->off = 0;
	buffer->size = 0;
	buffer->allocsize = expected_size;
	buffer->ops = &heap_buffer;
	buffer->heap = false;

	return 0;
}

void buffer_init_sink(struct buffer *buffer)
{
	buffer->data = NULL;
	buffer->off = 0;
	buffer->size = 0;
	buffer->allocsize = SIZE_MAX;
	buffer->ops = &sink_buffer;
	buffer->heap = false;
}

void buffer_init_static(struct buffer *buffer, const void *data, size_t size,
			size_t bufsize, bool writable)
{
	ASSERT3U(size, <=, bufsize);

	buffer->data = (void *) data;
	buffer->off = 0;
	buffer->size = size;
	buffer->allocsize = bufsize;
	buffer->ops = writable ? &static_buffer_rw : &static_buffer_ro;
	buffer->heap = false;
}

void buffer_init_stdio(struct buffer *buffer, FILE *f)
{
	buffer->data = NULL;
	buffer->off = 0;
	buffer->size = 0;
	buffer->allocsize = SIZE_MAX;
	buffer->ops = &stdio_buffer;
	buffer->private = f;
	buffer->heap = false;
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

	ret = resize(buffer, buffer->size + size);
	if (ret)
		return ret;

	buffer->ops->copyin(buffer, buffer->size, data, size);

	buffer->size += size;

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
			if (offset > buffer->size)
				return -EINVAL;

			newoff = offset;
			break;
		case SEEK_CUR:
			if ((offset > 0) &&
			    (offset > (buffer->size - buffer->off)))
				return -EINVAL;
			if ((offset < 0) && (-offset > buffer->off))
				return -EINVAL;

			newoff = buffer->off + offset;
			break;
		case SEEK_END:
			if (offset > 0)
				return -EINVAL;
			if (-offset > buffer->size)
				return -EINVAL;

			newoff = buffer->size + offset;
			break;
		default:
			return -EINVAL;
	}

	if (newoff > SSIZE_MAX)
		return -EOVERFLOW;

	if (buffer->ops->check_seek) {
		ssize_t ret;

		ret = buffer->ops->check_seek(buffer, offset, whence, newoff);
		if (ret)
			return ret;
	}

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

	/* clear any expansion */
	if (buffer->size < size)
		buffer->ops->clear(buffer, buffer->size, size - buffer->size);

	buffer->size = size;
	buffer->off = MIN(buffer->off, size);

	return 0;
}

ssize_t buffer_pread(struct buffer *buffer, void *buf, size_t len, size_t off)
{
	ssize_t ret;

	if (!buffer || !buf)
		return -EINVAL;

	if (buffer->ops->check_read) {
		ret = buffer->ops->check_read(buffer, buf, len, off);
		if (ret)
			return ret;
	}

	if (off >= buffer->size)
		ret = 0;
	else if ((off + len) > buffer->size)
		ret = buffer->size - off;
	else
		ret = len;

	if (ret)
		buffer->ops->copyout(buffer, off, buf, ret);

	return ret;
}

ssize_t buffer_pwrite(struct buffer *buffer, const void *buf, size_t len,
		      size_t off)
{
	ssize_t ret;

	if (!buffer || !buf)
		return -EINVAL;

	if (buffer->ops->check_write) {
		ret = buffer->ops->check_write(buffer, buf, len, off);
		if (ret)
			return ret;
	}

	ret = resize(buffer, off + len);
	if (ret)
		return ret;

	/* clear any holes */
	if (buffer->size < off)
		buffer->ops->clear(buffer, buffer->size, off - buffer->size);

	buffer->ops->copyin(buffer, off, buf, len);

	buffer->size = MAX(buffer->size, off + len);

	return len;
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

/* copyout implementations */
void generic_buffer_copyout_memcpy(struct buffer *buffer, size_t off,
				   void *data, size_t datalen)
{
	memcpy(data, buffer->data + off, datalen);
}

void generic_buffer_copyout_nop(struct buffer *buffer, size_t off, void *data,
				size_t datalen)
{
}

void generic_buffer_copyout_panic(struct buffer *buffer, size_t off, void *data,
				  size_t datalen)
{
	panic("buffer copyout called");
}
