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

#include <jeffpc/unicode.h>
#include <jeffpc/error.h>

/*
 * RFC 3629: UTF-8, a transformation format of ISO 10646
 */

/*
 * Convert a UTF-8 codepoint to a UCS4 codepoint and returned number of
 * consumed bytes.
 */
size_t utf8_to_utf32(const char *in, size_t inlen, uint32_t *out)
{
	uint32_t mincp, maxcp;
	unsigned char c;
	uint32_t res;
	size_t len;
	size_t i;

	if (!inlen)
		return 0; /* empty input */

	c = in[0];

	/* decode first byte */
	if ((c & 0xf8) == 0xf0) {
		len = 4;
		res = (c & 0x07) << 18;
		mincp = 0x010000;
		maxcp = 0x10ffff;
	} else if ((c & 0xf0) == 0xe0) {
		len = 3;
		res = (c & 0x0f) << 12;
		mincp = 0x0800;
		maxcp = 0xffff;
	} else if ((c & 0xe0) == 0xc0) {
		len = 2;
		res = (c & 0x1f) << 6;
		mincp = 0x080;
		maxcp = 0x7ff;
	} else if ((c & 0x80) == 0x00) {
		len = 1;
		res = c & 0x7f;
		mincp = 0x00;
		maxcp = 0x7f;
	} else {
		return 0; /* invalid first byte */
	}

	if (len > inlen)
		return 0; /* not enough bytes of input */

	for (i = 1; i < len; i++) {
		c = in[i];

		if ((c & 0xc0) != 0x80)
			return 0; /* invalid additional byte */

		res |= (c & 0x3f) << (6 * (len - i - 1));
	}

	/*
	 * final validity checks
	 */

	/* overlong sequences */
	if (res < mincp)
		return 0;
	/* codepoints > U+10FFFF */
	if (res > maxcp)
		return 0;
	if (!utf32_is_valid(res))
		return 0;

	*out = res;

	return len;
}

ssize_t utf32_to_utf8(uint32_t cp, char *buf, size_t buflen)
{
	ssize_t len;
	ssize_t i;

	if (!utf32_is_valid(cp))
		return -EINVAL; /* invalid codepoint */

	if (cp <= 0x7f)
		len = 1;
	else if (cp <= 0x7ff)
		len = 2;
	else if (cp <= 0xffff)
		len = 3;
	else
		len = 4;

	ASSERT3U(cp, <=, 0x10ffff);

	if (len > buflen)
		return -ENOMEM; /* not enough space */

	/* fast-path for ASCII */
	if (len == 1) {
		buf[0] = cp;
		return 1;
	}

	/* first byte */
	buf[0] = (cp >> (6 * (len - 1))) | (0xff << (8 - len));

	/* second...fourth byte */
	for (i = 1; i < len; i++)
		buf[i] = 0x80 | ((cp >> (6 * (len - i - 1))) & 0x3f);

	return len;
}

int utf8_is_valid_str(const char *src, size_t slen)
{
	size_t i = 0;

	while (i < slen) {
		uint32_t cp;
		size_t cplen;

		cplen = utf8_to_utf32(src + i, slen - i, &cp);
		if (!cplen)
			return -EILSEQ;

		i += cplen;
	}

	return 0;
}
