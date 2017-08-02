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

#include <jeffpc/types.h>
#include <jeffpc/error.h>
#include <jeffpc/urldecode.h>

#include "test.c"

struct test {
	const char *in;
	const char *out;
	ssize_t inlen;
	ssize_t outlen;
};

static const struct test input_tests[] = {
	{
		.in = NULL,
		.inlen = 1,
		.outlen = -EINVAL,
	},
	{
		.in = "",
		.out = "",
		.inlen = 0,
		.outlen = 0,
	},
	{
		.in = "a",
		.out = "a",
		.inlen = 1,
		.outlen = 1,
	},
	{
		.in = "ab",
		.out = "a",
		.inlen = 1,
		.outlen = 1,
	},
	{
		.in = "+",
		.out = " ",
		.inlen = 1,
		.outlen = 1,
	},
	{
		.in = "%20",
		.out = " ",
		.inlen = 3,
		.outlen = 1,
	},
	{
		.in = "%",
		.inlen = 1,
		.outlen = -EILSEQ,
	},
	{
		.in = "%0",
		.inlen = 1,
		.outlen = -EILSEQ,
	},
	{
		.in = "%0",
		.inlen = 2,
		.outlen = -EILSEQ,
	},
	{
		.in = "%00",
		.out = "\0",
		.inlen = 3,
		.outlen = 1,
	},
	{
		.in = "abcdefghijklmnopqrstuvwxyz"
		      "ABCDEFGHIJKLMNOPQRSTUVWXYZ"
		      "0123456789",
		.out = "abcdefghijklmnopqrstuvwxyz"
		       "ABCDEFGHIJKLMNOPQRSTUVWXYZ"
		       "0123456789",
		.inlen = 26 + 26 + 10,
		.outlen = 26 + 26 + 10,
	},
	{
		.in = "abc+def",
		.out = "abc def",
		.inlen = 7,
		.outlen = 7,
	},
	{
		.in = "abc=def",
		.out = "abc=def",
		.inlen = 7,
		.outlen = 7,
	},
	{
		.in = "abc&def",
		.out = "abc&def",
		.inlen = 7,
		.outlen = 7,
	},
	{
		.in = "abc%def",
		.out = "abc\xde""f",
		.inlen = 7,
		.outlen = 5,
	},
	{
		.in = "abc%DEf",
		.out = "abc\xde""f",
		.inlen = 7,
		.outlen = 5,
	},
	{
		.in = "abc%a0f",
		.out = "abc\xa0""f",
		.inlen = 7,
		.outlen = 5,
	},
	{
		.in = "abc%A0f",
		.out = "abc\xa0""f",
		.inlen = 7,
		.outlen = 5,
	},
	{
		.in = "abc%88f",
		.out = "abc\x88""f",
		.inlen = 7,
		.outlen = 5,
	},
};

static const struct test arg_tests[] = {
	{
		.in = NULL,
		.out = "",
		.inlen = 0,
		.outlen = -EINVAL,
	},
	{
		.in = "",
		.out = NULL,
		.inlen = 0,
		.outlen = -EINVAL,
	},
	{
		.in = NULL,
		.out = NULL,
		.inlen = 0,
		.outlen = -EINVAL,
	},
	{
		.in = NULL,
		.out = "",
		.inlen = 1,
		.outlen = -EINVAL,
	},
	{
		.in = "",
		.out = NULL,
		.inlen = 1,
		.outlen = -EINVAL,
	},
	{
		.in = NULL,
		.out = NULL,
		.inlen = 1,
		.outlen = -EINVAL,
	},
	{
		.in = "",
		.out = "",
		.inlen = 0,
		.outlen = 0,
	},
};

static char out[1024 * 1024];

static void test_args(void)
{
	int i;

	for (i = 0; i < ARRAY_LEN(arg_tests); i++) {
		const struct test *test = &arg_tests[i];
		ssize_t ret;

		fprintf(stderr, "%s: iter = %2d...", __func__, i);

		if (test->outlen > 0)
			fail("expected outlen %zd > 0", test->outlen);

		ret = urldecode(test->in, test->inlen, (char *) test->out);

		if ((ret > 0) && !test->outlen)
			fail("succeeded with %zd, should have succeeded with 0",
			     ret);

		if ((ret < 0) && !test->outlen)
			fail("failed with '%s', should have succeded with 0",
			     xstrerror(ret));

		if ((ret >= 0) && (test->outlen < 0))
			fail("succeeded with %zd, should have failed with %s",
			     ret, xstrerror(test->outlen));

		if (ret != test->outlen)
			fail("failed with '%s', should have failed with '%s'",
			     xstrerror(ret), xstrerror(test->outlen));

		fprintf(stderr, "ok.\n");
	}
}

static void test_inputs(void)
{
	int i;

	for (i = 0; i < ARRAY_LEN(input_tests); i++) {
		const struct test *test = &input_tests[i];
		ssize_t outlen;

		fprintf(stderr, "%s: iter = %2d...", __func__, i);

		if ((test->outlen >= 0) && (sizeof(out) < test->outlen))
			fail("output buffer is too small; "
			     "need %zd bytes, got %zu", test->outlen,
			     sizeof(out));

		outlen = urldecode(test->in, test->inlen, out);

		if ((outlen < 0) && (test->outlen >= 0))
			fail("urldecode failed with error: %s",
			     xstrerror(outlen));

		if ((outlen >= 0) && (test->outlen < 0))
			fail("urldecode returned %zd, should have failed: %s",
			     outlen, xstrerror(test->outlen));

		if ((outlen >= 0) && (outlen != test->outlen))
			fail("urldecode returned wrong number of bytes; "
			     "expected %zd, got %zd", test->outlen, outlen);

		if ((outlen < 0) && (outlen != test->outlen))
			fail("urldecode failed with wrong error; "
			     "expected '%s', got '%s'", xstrerror(test->outlen),
			     xstrerror(outlen));

		if ((outlen >= 0) && memcmp(out, test->out, outlen))
			fail("output doesn't match expected string; "
			     "expected '%*.*s', got '%*.*s'",
			     outlen, outlen, test->out,
			     outlen, outlen, out);

		fprintf(stderr, "ok.\n");
	}
}

void test(void)
{
	test_args();
	test_inputs();
}
