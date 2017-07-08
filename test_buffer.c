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

#include <jeffpc/buffer.h>
#include <jeffpc/error.h>

#include "test.c"

static inline void check_data(struct buffer *buffer)
{
	const void *ptr;

	ptr = buffer_data(buffer);
	if (ptr == NULL)
		fail("buffer_data() returned NULL");
	if (IS_ERR(ptr))
		fail("buffer_data() returned error: %s",
		     xstrerror(PTR_ERR(ptr)));
}

static inline void check_used(struct buffer *buffer, size_t expected)
{
	size_t got;

	got = buffer_used(buffer);
	if (got != expected)
		fail("buffer_used() == %zu, should be %zu", got, expected);
}

static inline void check_append(struct buffer *buffer, const void *ptr,
				size_t len)
{
	int ret;

	ret = buffer_append(buffer, ptr, len);
	if (ret)
		fail("buffer_append(..., %p, %zu) failed: %s", ptr, len,
		     xstrerror(ret));
}

static void test_alloc_free(void)
{
	struct buffer *buffer;
	size_t i;

	for (i = 0; i < 10; i++) {
		fprintf(stderr, "%s: iter = %d...", __func__, i);

		buffer = buffer_alloc(i);
		if (IS_ERR(buffer))
			fail("buffer_alloc(%zu) failed: %s", i,
			     xstrerror(PTR_ERR(buffer)));

		check_data(buffer);
		check_used(buffer, 0);

		buffer_free(buffer);

		fprintf(stderr, "\n");
	}
}

static void test_append(void)
{
	struct buffer *buffer;
	size_t startsize;
	size_t i;

	for (startsize = 0; startsize < 300; startsize++) {
		uint8_t data[256];

		fprintf(stderr, "%s: iter = %3d...", __func__, startsize);

		buffer = buffer_alloc(startsize);
		if (IS_ERR(buffer))
			fail("buffer_alloc(%zu) failed: %s", i,
			     xstrerror(PTR_ERR(buffer)));

		for (i = 0; i < 256; i++) {
			uint8_t byte = i;

			data[i] = i;

			check_data(buffer);
			check_used(buffer, i);
			check_append(buffer, &byte, 1);
			check_data(buffer);
			check_used(buffer, i + 1);
		}

		check_data(buffer);
		check_used(buffer, i);

		if (memcmp(data, buffer_data(buffer), sizeof(data)))
			fail("buffered data mismatches expectations");

		buffer_free(buffer);

		memset(data, 0, sizeof(data));

		fprintf(stderr, "\n");
	}
}

void test(void)
{
	test_alloc_free();
	test_append();
}
