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
	buffer->used = 0;
	buffer->allocsize = SIZE_MAX;
	buffer->ops = &sink_buffer;
}

void buffer_init_const(struct buffer *buffer, const void *data, size_t size)
{
	buffer->data = (void *) data;
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

	if ((data && !size) || (!data && size))
		return -EINVAL;

	if (buffer->ops->check_append) {
		ret = buffer->ops->check_append(buffer, data, size);
		if (ret)
			return ret;
	}

	if (!data && !size)
		return 0; /* append(..., NULL, 0) is a no-op */

	ret = resize(buffer, buffer->used + size);
	if (ret)
		return ret;

	buffer->ops->copyin(buffer, buffer->used, data, size);

	buffer->used += size;

	return 0;
}

/*
 * Generic implementations
 */

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