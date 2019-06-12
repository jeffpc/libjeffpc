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

static void check(const void *raw, size_t rawlen, const char *b64, size_t b64len,
		  bool url)
{
	/* +1 for \0, +10 as a redzone to catch out of bounds writes */
	char out[b64len + 1 + 10];

	memset(out, 0xfa, sizeof(out));

	fprintf(stderr, "inp: ");
	dumpraw(raw, rawlen);
	fprintf(stderr, "\n");
	fprintf(stderr, "exp: '%s'\n", b64);

	if (!url)
		base64_encode(out, raw, rawlen);
	else
		base64url_encode(out, raw, rawlen);

	fprintf(stderr, "got: '%s'\n", out);

	if (strlen(out) != b64len)
		fail("length mismatch (exp: %zu, got: %zu)", b64len,
		     strlen(out));

	if (memcmp(b64, out, b64len))
		fail("output mismatch");

	if (!check_padding(&out[b64len + 1], 0xfa, 10))
		fail("base64_encode wrote beyond end of buffer");
}

void test(const char *ifname, void *in, size_t ilen, const char *iext,
	  const char *ofname, void *out, size_t olen, const char *oext)
{
	check(in, ilen, out, olen, !strcmp(oext, "b64url"));
}
