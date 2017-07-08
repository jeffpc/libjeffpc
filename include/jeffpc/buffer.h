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

#ifndef __JEFFPC_BUFFER_H
#define __JEFFPC_BUFFER_H

#include <stdlib.h>

#include <jeffpc/error.h>

struct buffer {
	void *data;		/* the data itself */
	size_t used;		/* bytes filled */
	size_t allocsize;	/* allocated buffer size */
};

extern struct buffer *buffer_alloc(size_t expected_size);
extern void buffer_free(struct buffer *buffer);

/* returns number of bytes appended */
extern int buffer_append(struct buffer *buffer, const void *data, size_t size);

static inline size_t buffer_used(struct buffer *buffer)
{
	return buffer->used;
}

static inline const void *buffer_data(struct buffer *buffer)
{
	return buffer->data;
}

static inline int buffer_append_c(struct buffer *buffer, char c)
{
	return buffer_append(buffer, &c, 1);
}

static inline int buffer_append_str(struct buffer *buffer, const char *str)
{
	return buffer_append(buffer, str, strlen(str));
}

#endif
