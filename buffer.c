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

#include <jeffpc/buffer.h>

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
	buffer->sink = false;
	buffer->heap = true;

	return buffer;
}

void buffer_free(struct buffer *buffer)
{
	if (!buffer)
		return;

	free(buffer->data);
	free(buffer);
}

void buffer_init_sink(struct buffer *buffer)
{
	buffer->data = NULL;
	buffer->used = 0;
	buffer->allocsize = SIZE_MAX;
	buffer->sink = true;
	buffer->heap = false;

}

void buffer_init_const(struct buffer *buffer, const void *data, size_t size)
{
	buffer->data = (void *) data;
	buffer->used = size;
	buffer->allocsize = size;
	buffer->sink = false;
	buffer->heap = false;
}

static int resize(struct buffer *buffer, size_t newsize)
{
	void *tmp;

	ASSERT(!buffer->sink);
	ASSERT(buffer->heap);

	if (newsize <= buffer->allocsize)
		return 0;

	tmp = realloc(buffer->data, newsize);
	if (!tmp)
		return -ENOMEM;

	buffer->data = tmp;

	return 0;
}

int buffer_append(struct buffer *buffer, const void *data, size_t size)
{
	int ret;

	if (!buffer || (!buffer->sink && !buffer->heap))
		return -EINVAL;

	if (!data && !size)
		return 0; /* append(..., NULL, 0) is a no-op */

	if (!data || !size)
		return -EINVAL;

	if (!buffer->sink) {
		ret = resize(buffer, buffer->used + size);
		if (ret)
			return ret;

		memcpy(buffer->data + buffer->used, data, size);
	}

	buffer->used += size;

	return 0;
}
