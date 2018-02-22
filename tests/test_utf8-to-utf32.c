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
	const char *in;
	size_t inlen;
	size_t ret;
	uint32_t out;
};

#define DEF_TEST(r, o, s)	{ (s), sizeof(s) - 1, (r), (o) }

struct test runs[] = {
	/* test corner cases */
	DEF_TEST(1, 0x000000, "\0"),
	DEF_TEST(1, 0x00007f, "\x7f"),
	DEF_TEST(2, 0x000080, "\xc2\x80"),
	DEF_TEST(2, 0x0007ff, "\xdf\xbf"),
	DEF_TEST(3, 0x000800, "\xe0\xa0\x80"),
	DEF_TEST(3, 0x00ffff, "\xef\xbf\xbf"),
	DEF_TEST(4, 0x010000, "\xf0\x90\x80\x80"),
	DEF_TEST(4, 0x10ffff, "\xf4\x8f\xbf\xbf"),
	DEF_TEST(0, 0x110000, "\xf4\x90\x80\x80"),

	/* assorted ASCII chars */
	DEF_TEST(1, 0x000024, "$"),

	/* assorted 2-byte chars */
	DEF_TEST(2, 0x0000a2, "\xc2\xa2"),

	/* assorted 3-byte chars */
	DEF_TEST(3, 0x0020ac, "\xe2\x82\xac"),

	/* assorted 4-byte chars */
	DEF_TEST(4, 0x010348, "\xf0\x90\x8d\x88"),
	DEF_TEST(4, 0x01f600, "\xf0\x9f\x98\x80"),

	/* not reading more than needed */
	DEF_TEST(1, 0x000024, "$x"),
	DEF_TEST(2, 0x0000a2, "\xc2\xa2x"),
	DEF_TEST(3, 0x0020ac, "\xe2\x82\xacx"),
	DEF_TEST(4, 0x010348, "\xf0\x90\x8d\x88x"),

	/* truncated UTF-8 byte sequences */
	DEF_TEST(0, 0x0000a2, "\xc2"),
	DEF_TEST(0, 0x0020ac, "\xe2\x82"),
	DEF_TEST(0, 0x010348, "\xf0\x90\x8d"),

	/* UTF-16 surrogates */
	DEF_TEST(0, 0x00d800, "\xed\xa0\x80"),
	DEF_TEST(0, 0x00dfff, "\xed\xbf\xbf"),
	DEF_TEST(0, 0x0233b4, "\xed\xa1\x8c\xed\xbe\xb4"),

	/* overlong encoding */
	DEF_TEST(0, 0x000024, "\xc0\x80"),         /* '$' needs only 1 byte */
	DEF_TEST(0, 0x000024, "\xe0\x80\xa4"),     /* '$' needs only 1 byte */
	DEF_TEST(0, 0x000024, "\xf0\x80\x80\xa4"), /* '$' needs only 1 byte */
	DEF_TEST(0, 0x0000a2, "\xe0\x82\xa2"),     /* '¢' needs only 2 bytes */
	DEF_TEST(0, 0x0000a2, "\xf0\x80\x82\xa2"), /* '¢' needs only 2 bytes */
	DEF_TEST(0, 0x0020ac, "\xf0\x82\x82\xac"), /* '€' needs only 3 bytes */

	/* obsolete UTF-8 (more than 4 bytes) */
	DEF_TEST(0,   0x200000, "\xf8\xa0\x80\x80\x80"),     /* 5 bytes */
	DEF_TEST(0,  0x3ffffff, "\xfb\xbf\xbf\xbf\xbf"),     /* 5 bytes */
	DEF_TEST(0,  0x4000000, "\xfc\x84\x80\x80\x80\x80"), /* 6 bytes */
	DEF_TEST(0, 0x7fffffff, "\xfd\xbf\xbf\xbf\xbf\xbf"), /* 6 bytes */
};

static void __test(int idx, const char *in, size_t inlen, size_t exp_ret,
		   uint32_t exp_out)
{
	char hex[inlen * 2 + 1];
	uint32_t out;
	size_t ret;

	hexdumpz(hex, in, inlen, false);

	fprintf(stderr, "%2d: %12s -> U+%06x (%zu)...", idx, hex, exp_out,
		exp_ret);

	ret = utf8_to_utf32(in, inlen, &out);

	if (ret != exp_ret)
		fail("return mismatch: got %zu, expected %zu", ret, exp_ret);

	if (ret && (out != exp_out))
		fail("output mismatch: got %x, expected %x", out, exp_out);

	fprintf(stderr, "done.\n");
}

void test(void)
{
	int i;

	for (i = 0; i < ARRAY_LEN(runs); i++)
		__test(i, runs[i].in, runs[i].inlen, runs[i].ret, runs[i].out);
}
