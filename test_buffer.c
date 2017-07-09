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

static inline void check_data_ptr(struct buffer *buffer, const void *expected)
{
	const void *ptr;

	ptr = buffer_data(buffer);
	if (ptr == expected)
		return;

	if (IS_ERR(ptr))
		fail("buffer_data() returned error: %s (%p expected)",
		     xstrerror(PTR_ERR(ptr)), expected);
	fail("buffer_data() returned %p, but %p was expected", ptr, expected);
}

static inline void check_data_null(struct buffer *buffer)
{
	check_data_ptr(buffer, NULL);
}

static inline void check_used(struct buffer *buffer, size_t expected)
{
	size_t got;

	got = buffer_used(buffer);
	if (got != expected)
		fail("buffer_used() == %zu, should be %zu", got, expected);
}

static inline void check_append_err(struct buffer *buffer, const void *ptr,
				    size_t len, int expected_ret)
{
	int ret;

	ret = buffer_append(buffer, ptr, len);
	if (ret == expected_ret)
		return;

	if (ret && expected_ret)
		fail("buffer_append(..., %p, %zu) failed with wrong error: "
		     "got %s, expected %s", ptr, len, xstrerror(ret),
		     xstrerror(expected_ret));
	if (ret && !expected_ret)
		fail("buffer_append(..., %p, %zu) failed but it wasn't "
		     "supposed to: %s", ptr, len, xstrerror(ret));
	if (!ret && expected_ret)
		fail("buffer_append(..., %p, %zu) succeeded but it wasn't "
		     "supposed to. Expected error: %s", ptr, len,
		     xstrerror(expected_ret));
	fail("impossible condition occured");
}

static inline void check_append(struct buffer *buffer, const void *ptr,
				size_t len)
{
	check_append_err(buffer, ptr, len, 0);
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

static void inner_loop(size_t niter, struct buffer *buffer, uint8_t *data,
		       void (*check)(struct buffer *))
{
	size_t i;

	for (i = 0; i < niter; i++) {
		uint8_t byte = i;

		if (data)
			data[i] = i;

		check(buffer);
		check_used(buffer, i);
		check_append(buffer, &byte, 1);
		check(buffer);
		check_used(buffer, i + 1);
	}

	check(buffer);
	check_used(buffer, i);
}

static void test_append(void)
{
	struct buffer *buffer;
	size_t startsize;

	for (startsize = 0; startsize < 300; startsize++) {
		uint8_t data[256];

		fprintf(stderr, "%s: iter = %3d...", __func__, startsize);

		buffer = buffer_alloc(startsize);
		if (IS_ERR(buffer))
			fail("buffer_alloc(%zu) failed: %s", startsize,
			     xstrerror(PTR_ERR(buffer)));

		inner_loop(256, buffer, data, check_data);

		if (memcmp(data, buffer_data(buffer), sizeof(data)))
			fail("buffered data mismatches expectations");

		buffer_free(buffer);

		memset(data, 0, sizeof(data));

		fprintf(stderr, "ok.\n");
	}
}

static void test_sink(void)
{
	struct buffer buffer;
	size_t maxsize;

	for (maxsize = 0; maxsize < 270; maxsize++) {
		buffer_init_sink(&buffer);

		fprintf(stderr, "%s: iter = %3d...", __func__, maxsize);

		inner_loop(256, &buffer, NULL, check_data_null);

		memset(&buffer, 0, sizeof(struct buffer));

		fprintf(stderr, "ok.\n");
	}

	for (maxsize = 300; maxsize < 300000; maxsize += 10000) {
		buffer_init_sink(&buffer);

		fprintf(stderr, "%s: iter = %3d...", __func__, maxsize);

		inner_loop(256, &buffer, NULL, check_data_null);

		memset(&buffer, 0, sizeof(struct buffer));

		fprintf(stderr, "ok.\n");
	}
}

void test_const(void)
{
	const char rawdata[] = "759f7e2d-67ec-4e72-8f61-86a3fd93b1be"
			       "60e9149e-d039-e32b-b25d-c995b28bf890"
			       "40f0fddc-ddca-4ff5-cd81-b0ae4c7d6123";
	struct buffer buffer;
	int i;

	buffer_init_const(&buffer, rawdata, strlen(rawdata));

	check_used(&buffer, strlen(rawdata));
	check_data_ptr(&buffer, rawdata);

	for (i = 0; i < 10; i++) {
		fprintf(stderr, "%s: iter = %d...", __func__, i);

		check_append_err(&buffer, "abc", 3, -EINVAL);
		check_used(&buffer, strlen(rawdata));
		check_data_ptr(&buffer, rawdata);

		fprintf(stderr, "ok.\n");
	}
}

void test(void)
{
	test_alloc_free();
	test_append();
	test_sink();
	test_const();
}
