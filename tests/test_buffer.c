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

#include <jeffpc/buffer.h>
#include <jeffpc/error.h>
#include <jeffpc/rand.h>

#include "test.c"

#define COMMON_TEST_STRING \
	"759f7e2d-67ec-4e72-8f61-86a3fd93b1be" \
	"60e9149e-d039-e32b-b25d-c995b28bf890" \
	"40f0fddc-ddca-4ff5-cd81-b0ae4c7d6123"

/* allocate a heap buffer - either on the heap or on the stack */
static inline struct buffer *alloc_heap_buffer(struct buffer *buf, size_t size)
{
	if (buf) {
		int ret;

		ret = buffer_init_heap(buf, size);
		if (ret)
			fail("buffer_init_heap(%zu) failed: %s", size,
			     xstrerror(ret));
	} else {
		buf = buffer_alloc(size);
		if (IS_ERR(buf))
			fail("buffer_alloc(%zu) failed: %s", size,
			     xstrerror(PTR_ERR(buf)));

	}

	return buf;
}

static inline void check_data_zeroes(struct buffer *buffer, size_t startoff)
{
	const uint8_t *data;
	size_t len;
	size_t i;

	data = buffer_data(buffer);
	len = buffer_size(buffer);

	if (startoff > len)
		fail("%s startoff > len (%zu > %zu)", __func__, startoff, len);

	for (i = startoff; i < len; i++) {
		if (data[i] == '\0')
			continue;

		fail("buffer contains %#02x @ offset %zu (should be '\\0')",
		     data[i], i);
	}
}

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

	got = buffer_size(buffer);
	if (got != expected)
		fail("buffer_size() == %zu, should be %zu", got, expected);
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

static inline void check_truncate_err(struct buffer *buffer, size_t newsize,
				      int expected_ret)
{
	int ret;

	ret = buffer_truncate(buffer, newsize);
	if (ret == expected_ret)
		return;

	if (ret && expected_ret)
		fail("buffer_truncate(..., %zu) failed with wrong error: "
		     "got %s, expected %s", newsize, xstrerror(ret),
		     xstrerror(expected_ret));
	if (ret && !expected_ret)
		fail("buffer_truncate(..., %zu) failed but it wasn't "
		     "supposed to: %s", newsize, xstrerror(ret));
	if (!ret && expected_ret)
		fail("buffer_truncate(..., %zu) succeeded but it wasn't "
		     "supposed to. Expected error: %s", newsize,
		     xstrerror(expected_ret));
	fail("impossible condition occured");
}

static inline void check_truncate(struct buffer *buffer, size_t newsize)
{
	check_truncate_err(buffer, newsize, 0);
}

static void test_alloc_free(struct buffer *_buffer)
{
	struct buffer *buffer;
	size_t i;

	for (i = 0; i < 10; i++) {
		fprintf(stderr, "%s(%p): iter = %zu...", __func__, _buffer, i);

		buffer = alloc_heap_buffer(_buffer, i);

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

		/* truncate to same size */
		check_truncate(buffer, buffer_size(buffer));
		check(buffer);
		check_used(buffer, i + 1);
	}

	check(buffer);
	check_used(buffer, i);
}

static void test_append(struct buffer *_buffer)
{
	struct buffer *buffer;
	size_t startsize;

	for (startsize = 0; startsize < 300; startsize++) {
		uint8_t data[256];

		fprintf(stderr, "%s(%p): iter = %3zu...", __func__, _buffer,
			startsize);

		buffer = alloc_heap_buffer(_buffer, startsize);

		inner_loop(256, buffer, data, check_data);

		if (memcmp(data, buffer_data(buffer), sizeof(data)))
			fail("buffered data mismatches expectations");

		buffer_free(buffer);

		memset(data, 0, sizeof(data));

		fprintf(stderr, "ok.\n");
	}
}

static void test_truncate_grow(struct buffer *_buffer)
{
	struct buffer *buffer;
	size_t i;

	buffer = alloc_heap_buffer(_buffer, 0);

	for (i = 0; i < 50000; i += 13) {
		fprintf(stderr, "%s: iter = %3zu...", __func__, i);

		check_truncate(buffer, i);
		check_used(buffer, i);
		check_data_zeroes(buffer, 0);

		fprintf(stderr, "ok.\n");
	}

	buffer_free(buffer);
}

static void test_truncate_shrink(struct buffer *_buffer)
{
	const size_t maxsize = 5000 * sizeof(uint64_t);
	struct buffer *buffer;
	size_t i;

	buffer = alloc_heap_buffer(_buffer, maxsize);

	/* append some random data */
	for (i = 0; i < maxsize; i += sizeof(uint64_t)) {
		uint64_t tmp;

		tmp = rand64();

		check_append(buffer, &tmp, sizeof(tmp));
	}

	/* sanity check the size */
	check_used(buffer, maxsize);

	for (i = maxsize; i > 0; i -= sizeof(uint64_t)) {
		fprintf(stderr, "%s: iter = %3zu...", __func__, i);

		check_truncate(buffer, i);
		check_used(buffer, i);

		fprintf(stderr, "ok.\n");
	}

	buffer_free(buffer);
}

static void test_sink(void)
{
	struct buffer buffer;
	size_t maxsize;

	for (maxsize = 0; maxsize < 270; maxsize++) {
		buffer_init_sink(&buffer);

		fprintf(stderr, "%s: iter = %3zu...", __func__, maxsize);

		inner_loop(256, &buffer, NULL, check_data_null);

		memset(&buffer, 0, sizeof(struct buffer));

		fprintf(stderr, "ok.\n");
	}

	for (maxsize = 300; maxsize < 300000; maxsize += 10000) {
		buffer_init_sink(&buffer);

		fprintf(stderr, "%s: iter = %3zu...", __func__, maxsize);

		inner_loop(256, &buffer, NULL, check_data_null);

		memset(&buffer, 0, sizeof(struct buffer));

		fprintf(stderr, "ok.\n");
	}
}

void test_static_const_arg(void)
{
	const char const_data[] = "abc";
	struct buffer buffer;

	/* (statically) check for passing in const pointer being ok */
	buffer_init_static(&buffer, const_data, strlen(const_data),
			   strlen(const_data), false);
}

void test_static_ro(void)
{
	const char rawdata[] = COMMON_TEST_STRING;
	struct buffer buffer;
	int i;

	buffer_init_static(&buffer, rawdata, strlen(rawdata), strlen(rawdata),
			   false);

	check_used(&buffer, strlen(rawdata));
	check_data_ptr(&buffer, rawdata);

	for (i = 0; i < 10; i++) {
		fprintf(stderr, "%s: iter = %d...", __func__, i);

		check_append_err(&buffer, "abc", 3, -EROFS);
		check_used(&buffer, strlen(rawdata));
		check_data_ptr(&buffer, rawdata);

		check_truncate_err(&buffer, i * strlen(rawdata) / 5, -EROFS);
		check_used(&buffer, strlen(rawdata));
		check_data_ptr(&buffer, rawdata);

		fprintf(stderr, "ok.\n");
	}
}

void test_static_rw(void)
{
	char rawdata[] = COMMON_TEST_STRING;
	const size_t rawlen = strlen(rawdata);
	struct buffer buffer;
	size_t size;

	for (size = 0; size <= rawlen; size++) {
		buffer_init_static(&buffer, rawdata, size, rawlen, true);

		fprintf(stderr, "%s(%zu): initial sanity check...", __func__,
			size);
		check_used(&buffer, size);
		check_data_ptr(&buffer, rawdata);
		fprintf(stderr, "ok.\n");

		fprintf(stderr, "%s(%zu): append to a buffer without enough "
			"space...", __func__, size);
		check_append_err(&buffer, COMMON_TEST_STRING, rawlen + 1,
				 -ENOSPC);
		check_used(&buffer, size);
		check_data_ptr(&buffer, rawdata);
		fprintf(stderr, "ok.\n");

		fprintf(stderr, "%s(%zu): truncate to a larger than buffer "
			"size...", __func__, size);
		check_truncate_err(&buffer, rawlen + 1, -ENOSPC);
		check_used(&buffer, size);
		check_data_ptr(&buffer, rawdata);
		fprintf(stderr, "ok.\n");

		if (size >= 5) {
			fprintf(stderr, "%s(%zu): truncate to a smaller than "
				"initial size...", __func__, size);
			check_truncate_err(&buffer, size - 5, 0);
			check_used(&buffer, size - 5);
			check_data_ptr(&buffer, rawdata);
			fprintf(stderr, "ok.\n");

			fprintf(stderr, "%s(%zu): append too much...", __func__, size);
			check_append_err(&buffer, COMMON_TEST_STRING COMMON_TEST_STRING,
					 rawlen - size + 11, -ENOSPC);
			check_used(&buffer, size - 5);
			check_data_ptr(&buffer, rawdata);
			fprintf(stderr, "ok.\n");

			fprintf(stderr, "%s(%zu): append a little...", __func__, size);
			check_append_err(&buffer, COMMON_TEST_STRING, 5, 0);
			check_used(&buffer, size);
			check_data_ptr(&buffer, rawdata);
			fprintf(stderr, "ok.\n");
		}
	}
}

void test(void)
{
	struct buffer stack_buf;

	/* stack allocated buffer struct */
	test_alloc_free(&stack_buf);
	test_append(&stack_buf);
	test_truncate_grow(&stack_buf);
	test_truncate_shrink(&stack_buf);

	/* heap allocated buffer struct */
	test_alloc_free(NULL);
	test_append(NULL);
	test_truncate_grow(NULL);
	test_truncate_shrink(NULL);

	test_sink();
	test_static_const_arg();
	test_static_ro();
	test_static_rw();
}
