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

#include <jeffpc/array.h>
#include <jeffpc/error.h>

#include "test.c"

static void test_alloc_free(void)
{
	int *arr;

	arr = array_alloc(sizeof(int), 0);
	if (!arr)
		fail("array_alloc returned NULL");

	array_free(arr);
}

#define CHECK_SIZE(arr, expected_size)					\
	do {								\
		size_t exp = (expected_size);				\
		size_t got;						\
									\
		fprintf(stderr, "checking array size; expect %zu...",	\
			exp);						\
									\
		got = array_size(arr);					\
									\
		fprintf(stderr, "got %zu - ", got);			\
									\
		if (got != exp)						\
			fail("size mismatch! expected %zu, got %zu",	\
			     exp, got);					\
									\
		fprintf(stderr, "ok\n");				\
	} while (0)

#define CHECK_VAL(arr, idx, expected_val)				\
	do {								\
		size_t exp = (expected_val);				\
									\
		fprintf(stderr, "checking array idx %zu; expect %zu...",\
			(idx), exp);					\
									\
		if (arr[idx] != (expected_val))				\
			fail("value mismatch! expected %zu, got %u",	\
			     exp, arr[i]);				\
									\
		fprintf(stderr, "ok\n");				\
	} while (0)

#define GEN_VAL(i)	((i) * 10 + 7)

static void test_size(void)
{
	unsigned int *arr;
	size_t i, j, k;

	for (i = 0; i < 3; i++) {
		fprintf(stderr, "prealloc %zu\n", i * 10);

		arr = array_alloc(sizeof(int), i * 10);
		if (!arr)
			fail("array_alloc_returned NULL");

		CHECK_SIZE(arr, 0);

		for (j = 0; j < 20; j++) {
			int ret;

			fprintf(stderr, "truncating to %zu\n", j);

			ret = array_truncate(&arr, j);
			if (ret)
				fail("truncate failed: %s", xstrerror(ret));

			CHECK_SIZE(arr, j);

			if (j) {
				/* check the padding */
				CHECK_VAL(arr, j - 1, 0);

				/* set the newly allocated value */
				arr[j - 1] = GEN_VAL(j - 1);
			}

			/* check that the previous values are still there */
			for (k = 0; k < j; k++)
				CHECK_VAL(arr, k, GEN_VAL(k));
		}

		array_free(arr);
	}
}

void test(void)
{
	test_alloc_free();
	test_size();
}
