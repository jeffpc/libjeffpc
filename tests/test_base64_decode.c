/*
 * Copyright (c) 2019 Josef 'Jeff' Sipek <jeffpc@josefsipek.net>
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

#include <jeffpc/base64.h>
#include <jeffpc/padding.h>
#include <jeffpc/hexdump.h>
#include <jeffpc/io.h>

#include "test.c"

static inline void dumpraw(const void *buf, size_t len)
{
	char tmp[len * 2 + 1];

	hexdumpz(tmp, buf, len, false);
	fprintf(stderr, "%s", tmp);
}

static void check(const void *raw, size_t rawlen, const char *b64, size_t b64len)
{
	/* +10 as a redzone to catch out of bounds writes */
	uint8_t out[rawlen + 10];
	ssize_t ret;

	memset(out, 0xfa, sizeof(out));

	fprintf(stderr, "inp: '%s'\n", b64);
	fprintf(stderr, "exp: ");
	dumpraw(raw, rawlen);
	fprintf(stderr, "\n");

	ret = base64_decode(out, b64, b64len);
	if (ret < 0)
		fail("failed to decode");

	fprintf(stderr, "got: ");
	dumpraw(out, ret);
	fprintf(stderr, "\n");

	if (ret != rawlen)
		fail("length mismatch (exp: %zu, got: %zu)", rawlen, ret);

	if (!check_padding(&out[rawlen], 0xfa, 10))
		fail("base64_encode wrote beyond end of buffer");
}

void test(const char *fname)
{
	char expfname[FILENAME_MAX];
	size_t rawlen;
	size_t b64len;
	void *raw;
	char *b64;

	/* replace .b64 with .raw */
	strcpy(expfname, fname);
	strcpy(expfname + strlen(expfname) - 3, "raw");

	/* read input */
	b64 = read_file_len(fname, &b64len);
	ASSERT(!IS_ERR(b64));

	/* read expected output */
	raw = read_file_len(expfname, &rawlen);
	ASSERT(!IS_ERR(raw));

	check(raw, rawlen, b64, b64len);
}
