/*
 * Copyright (c) 2018 Josef 'Jeff' Sipek <jeffpc@josefsipek.net>
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
#include <jeffpc/hexdump.h>
#include <jeffpc/unicode.h>

#include "test.c"

struct test {
	uint32_t in;
	ssize_t ret;
	const char *out;
};

#define DEF_TEST(c, r, s)	{ (c), (r), (s) }

struct test runs[] = {
	/* test corner cases */
	DEF_TEST(0x000000, 1, "\0"),
	DEF_TEST(0x00007f, 1, "\x7f"),
	DEF_TEST(0x000080, 2, "\xc2\x80"),
	DEF_TEST(0x0007ff, 2, "\xdf\xbf"),
	DEF_TEST(0x000800, 3, "\xe0\xa0\x80"),
	DEF_TEST(0x00ffff, 3, "\xef\xbf\xbf"),
	DEF_TEST(0x010000, 4, "\xf0\x90\x80\x80"),
	DEF_TEST(0x10ffff, 4, "\xf4\x8f\xbf\xbf"),
	DEF_TEST(0x110000, -EINVAL, NULL),

	/* assorted ASCII chars */
	DEF_TEST(0x000024, 1, "$"),

	/* assorted 2-byte chars */
	DEF_TEST(0x0000a2, 2, "\xc2\xa2"),

	/* assorted 3-byte chars */
	DEF_TEST(0x0020ac, 3, "\xe2\x82\xac"),

	/* assorted 4-byte chars */
	DEF_TEST(0x010348, 4, "\xf0\x90\x8d\x88"),
	DEF_TEST(0x01f600, 4, "\xf0\x9f\x98\x80"),
	DEF_TEST(0x0233b4, 4, "\xf0\xa3\x8e\xb4"),

	/* UTF-16 surrogates */
	DEF_TEST(0x00d800, -EINVAL, NULL),
	DEF_TEST(0x00dfff, -EINVAL, NULL),

	/* obsolete UTF-8 (more than 4 bytes) */
	DEF_TEST(  0x200000, -EINVAL, NULL), /* 5 bytes */
	DEF_TEST( 0x3ffffff, -EINVAL, NULL), /* 5 bytes */
	DEF_TEST( 0x4000000, -EINVAL, NULL), /* 6 bytes */
	DEF_TEST(0x7fffffff, -EINVAL, NULL), /* 6 bytes */
};

static void printout(int idx, const char *pfx, const void *raw, ssize_t len)
{
	fprintf(stderr, "%2d:  %8s: ", idx, pfx);

	if (len >= 0) {
		char hex[len * 2 + 1];

		hexdumpz(hex, raw, len, false);
		fprintf(stderr, "%zd bytes: %s\n", len, hex);
	} else {
		fprintf(stderr, "error: %s\n", xstrerror(len));
	}
}

static void __test(int idx, uint32_t in, ssize_t exp_ret, const char *exp_out)
{
	char out[10];
	ssize_t ret;

	memset(out, 0x5a, sizeof(out));

	fprintf(stderr, "%2d: U+%06x...\n", idx, in);
	printout(idx, "expected", exp_out, exp_ret);

	ret = utf32_to_utf8(in, out, sizeof(out));
	printout(idx, "got", out, ret);

	if (ret != exp_ret) {
		if ((ret < 0) && (exp_ret < 0))
			fail("return mismatch: got '%s', expected '%s'",
			     xstrerror(ret), xstrerror(exp_ret));

		if ((ret < 0) && (exp_ret >= 0))
			fail("return mismatch: got '%s', expected %zd",
			     xstrerror(ret), exp_ret);

		if ((ret >= 0) && (exp_ret < 0))
			fail("return mismatch: got %zd, expected '%s'",
			     ret, xstrerror(exp_ret));

		if ((ret >= 0) && (exp_ret >= 0))
			fail("return mismatch: got %zd, expected %zd",
			     ret, exp_ret);
	}

	if (ret > 0 && memcmp(out, exp_out, exp_ret))
		fail("output mismatch!");

	if (exp_ret > 0) {
		ret = utf32_to_utf8(in, out, exp_ret - 1);

		if (ret != -ENOMEM)
			fail("return mismatch: got %zu, expected %d "
			     "(-ENOMEM)", ret, exp_ret);
	}

	fprintf(stderr, "%2d: done.\n", idx);
}

void test(void)
{
	int i;

	for (i = 0; i < ARRAY_LEN(runs); i++)
		__test(i, runs[i].in, runs[i].ret, runs[i].out);
}
