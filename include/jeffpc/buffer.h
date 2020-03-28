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

#ifndef __JEFFPC_BUFFER_H
#define __JEFFPC_BUFFER_H

#include <stdio.h>
#include <stdlib.h>
#include <stdbool.h>
#include <fcntl.h>

#include <jeffpc/error.h>
#include <jeffpc/val.h>

struct buffer;

struct buffer_ops {
	/* op checking */
	int (*check_append)(struct buffer *, const void *, size_t);
	int (*check_read)(struct buffer *, void *, size_t, size_t);
	int (*check_truncate)(struct buffer *, size_t);
	int (*check_write)(struct buffer *, const void *, size_t, size_t);
	ssize_t (*check_seek)(struct buffer *, off_t, int, size_t);

	/* data manipulation */
	void *(*realloc)(void *, size_t);
	void (*free)(void *);
	void (*clear)(struct buffer *, size_t, size_t);
	void (*copyin)(struct buffer *, size_t, const void *, size_t);
	void (*copyout)(struct buffer *, size_t, void *, size_t);
};

struct buffer {
	void *data;		/* the data itself */
	size_t off;		/* current pointer */
	size_t size;		/* bytes filled */
	size_t allocsize;	/* allocated buffer size */
	const struct buffer_ops *ops;
	void *private;		/* private data for implementation */
	bool heap:1;		/* struct buffer is on the heap */
};

extern struct buffer *buffer_alloc(size_t expected_size);
extern void buffer_free(struct buffer *buffer);

/* a buffer that behaves similarly to /dev/null - all appends are lost */
extern void buffer_init_sink(struct buffer *buffer);
/*
 * a buffer that wraps a data pointer (we use const void * to allow passing
 * in both void * and const void *)
 */
extern void buffer_init_static(struct buffer *buffer, const void *data,
			       size_t size, bool writable);
/* a buffer that writes to a FILE * */
extern void buffer_init_stdio(struct buffer *buffer, FILE *f);

/* returns number of bytes appended */
extern int buffer_append(struct buffer *buffer, const void *data, size_t size);
extern ssize_t buffer_seek(struct buffer *buffer, off_t offset, int whence);
extern int buffer_truncate(struct buffer *buffer, size_t size);
extern ssize_t buffer_pread(struct buffer *buffer, void *data, size_t len,
			    size_t off);
extern ssize_t buffer_pwrite(struct buffer *buffer, const void *data, size_t len,
			     size_t off);

static inline size_t buffer_size(struct buffer *buffer)
{
	return buffer->size;
}

/* number of bytes between current location and the end of data */
static inline size_t buffer_remain(struct buffer *buffer)
{
	return buffer->size - buffer->off;
}

static inline size_t buffer_offset(struct buffer *buffer)
{
	return buffer->off;
}

static inline const void *buffer_data(struct buffer *buffer)
{
	return buffer->data;
}

static inline const void *buffer_data_current(struct buffer *buffer)
{
	if (!buffer->data)
		return NULL;
	if (buffer->off == buffer->size)
		return NULL;
	return buffer->data + buffer->off;
}

static inline int buffer_append_c(struct buffer *buffer, char c)
{
	return buffer_append(buffer, &c, 1);
}

static inline int buffer_append_cstr(struct buffer *buffer, const char *str)
{
	return buffer_append(buffer, str, strlen(str));
}

static inline int buffer_append_str(struct buffer *buffer, const struct str *s)
{
	const char *str = str_cstr(s);

	if (!str)
		return 0;

	return buffer_append_cstr(buffer, str);
}

/* same as buffer_pread(), but it uses and updates current offset */
static inline ssize_t buffer_read(struct buffer *buffer, void *buf, size_t len)
{
	ssize_t ret;

	ret = buffer_pread(buffer, buf, len, buffer->off);

	if (ret >= 0)
		buffer->off += ret;

	return ret;
}

/* same as buffer_pwrite(), but it uses and updates current offset */
static inline ssize_t buffer_write(struct buffer *buffer, const void *buf,
				   size_t len)
{
	ssize_t ret;

	ret = buffer_pwrite(buffer, buf, len, buffer->off);

	if (ret >= 0)
		buffer->off += ret;

	return ret;
}

#endif
