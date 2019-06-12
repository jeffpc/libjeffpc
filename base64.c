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

static const char b64_encode_table[64] =
	"ABCDEFGHIJKLMNOPQRSTUVWXYZ"
	"abcdefghijklmnopqrstuvwxyz"
	"0123456789+/";

static const char b64url_encode_table[64] =
	"ABCDEFGHIJKLMNOPQRSTUVWXYZ"
	"abcdefghijklmnopqrstuvwxyz"
	"0123456789-_";

#define INV	0xff

/*
 * Check for any one of the four chars being INV.
 *
 * Optimization: Since no valid char translates to a value with with 0x40 or
 * 0x80 set, the only way to get 0xff out is if one of the four values was
 * 0xff - aka. INV.
 */
#define CHECK_INV(a, b, c, d)	(((a) | (b) | (c) | (d)) == INV)

static const uint8_t b64_decode_table[256] = {
	 INV,  INV,  INV,  INV,  INV,  INV,  INV,  INV, /* 0x00..0x07 */
	 INV,  INV,  INV,  INV,  INV,  INV,  INV,  INV, /* 0x08..0x0f */
	 INV,  INV,  INV,  INV,  INV,  INV,  INV,  INV, /* 0x10..0x17 */
	 INV,  INV,  INV,  INV,  INV,  INV,  INV,  INV, /* 0x18..0x1f */
	 INV,  INV,  INV,  INV,  INV,  INV,  INV,  INV, /* 0x20..0x27 */
	 INV,  INV,  INV, 0x3e,  INV,  INV,  INV, 0x3f, /* 0x28..0x2f */
	0x34, 0x35, 0x36, 0x37, 0x38, 0x39, 0x3a, 0x3b, /* 0x30..0x37 */
	0x3c, 0x3d,  INV,  INV,  INV,  INV,  INV,  INV, /* 0x38..0x3f */
	 INV, 0x00, 0x01, 0x02, 0x03, 0x04, 0x05, 0x06, /* 0x40..0x47 */
	0x07, 0x08, 0x09, 0x0a, 0x0b, 0x0c, 0x0d, 0x0e, /* 0x48..0x4f */
	0x0f, 0x10, 0x11, 0x12, 0x13, 0x14, 0x15, 0x16, /* 0x50..0x57 */
	0x17, 0x18, 0x19,  INV,  INV,  INV,  INV,  INV, /* 0x58..0x5f */
	 INV, 0x1a, 0x1b, 0x1c, 0x1d, 0x1e, 0x1f, 0x20, /* 0x60..0x67 */
	0x21, 0x22, 0x23, 0x24, 0x25, 0x26, 0x27, 0x28, /* 0x68..0x6f */
	0x29, 0x2a, 0x2b, 0x2c, 0x2d, 0x2e, 0x2f, 0x30, /* 0x70..0x77 */
	0x31, 0x32, 0x33,  INV,  INV,  INV,  INV,  INV, /* 0x78..0x7f */
	 INV,  INV,  INV,  INV,  INV,  INV,  INV,  INV, /* 0x80..0x87 */
	 INV,  INV,  INV,  INV,  INV,  INV,  INV,  INV, /* 0x88..0x8f */
	 INV,  INV,  INV,  INV,  INV,  INV,  INV,  INV, /* 0x90..0x97 */
	 INV,  INV,  INV,  INV,  INV,  INV,  INV,  INV, /* 0x98..0x9f */
	 INV,  INV,  INV,  INV,  INV,  INV,  INV,  INV, /* 0xa0..0xa7 */
	 INV,  INV,  INV,  INV,  INV,  INV,  INV,  INV, /* 0xa8..0xaf */
	 INV,  INV,  INV,  INV,  INV,  INV,  INV,  INV, /* 0xb0..0xb7 */
	 INV,  INV,  INV,  INV,  INV,  INV,  INV,  INV, /* 0xb8..0xbf */
	 INV,  INV,  INV,  INV,  INV,  INV,  INV,  INV, /* 0xc0..0xc7 */
	 INV,  INV,  INV,  INV,  INV,  INV,  INV,  INV, /* 0xc8..0xcf */
	 INV,  INV,  INV,  INV,  INV,  INV,  INV,  INV, /* 0xd0..0xd7 */
	 INV,  INV,  INV,  INV,  INV,  INV,  INV,  INV, /* 0xd8..0xdf */
	 INV,  INV,  INV,  INV,  INV,  INV,  INV,  INV, /* 0xe0..0xe7 */
	 INV,  INV,  INV,  INV,  INV,  INV,  INV,  INV, /* 0xe8..0xef */
	 INV,  INV,  INV,  INV,  INV,  INV,  INV,  INV, /* 0xf0..0xf7 */
	 INV,  INV,  INV,  INV,  INV,  INV,  INV,  INV, /* 0xf8..0xff */
};

static const uint8_t b64url_decode_table[256] = {
	 INV,  INV,  INV,  INV,  INV,  INV,  INV,  INV, /* 0x00..0x07 */
	 INV,  INV,  INV,  INV,  INV,  INV,  INV,  INV, /* 0x08..0x0f */
	 INV,  INV,  INV,  INV,  INV,  INV,  INV,  INV, /* 0x10..0x17 */
	 INV,  INV,  INV,  INV,  INV,  INV,  INV,  INV, /* 0x18..0x1f */
	 INV,  INV,  INV,  INV,  INV,  INV,  INV,  INV, /* 0x20..0x27 */
	 INV,  INV,  INV,  INV,  INV, 0x3e,  INV,  INV, /* 0x28..0x2f */
	0x34, 0x35, 0x36, 0x37, 0x38, 0x39, 0x3a, 0x3b, /* 0x30..0x37 */
	0x3c, 0x3d,  INV,  INV,  INV,  INV,  INV,  INV, /* 0x38..0x3f */
	 INV, 0x00, 0x01, 0x02, 0x03, 0x04, 0x05, 0x06, /* 0x40..0x47 */
	0x07, 0x08, 0x09, 0x0a, 0x0b, 0x0c, 0x0d, 0x0e, /* 0x48..0x4f */
	0x0f, 0x10, 0x11, 0x12, 0x13, 0x14, 0x15, 0x16, /* 0x50..0x57 */
	0x17, 0x18, 0x19,  INV,  INV,  INV,  INV, 0x3f, /* 0x58..0x5f */
	 INV, 0x1a, 0x1b, 0x1c, 0x1d, 0x1e, 0x1f, 0x20, /* 0x60..0x67 */
	0x21, 0x22, 0x23, 0x24, 0x25, 0x26, 0x27, 0x28, /* 0x68..0x6f */
	0x29, 0x2a, 0x2b, 0x2c, 0x2d, 0x2e, 0x2f, 0x30, /* 0x70..0x77 */
	0x31, 0x32, 0x33,  INV,  INV,  INV,  INV,  INV, /* 0x78..0x7f */
	 INV,  INV,  INV,  INV,  INV,  INV,  INV,  INV, /* 0x80..0x87 */
	 INV,  INV,  INV,  INV,  INV,  INV,  INV,  INV, /* 0x88..0x8f */
	 INV,  INV,  INV,  INV,  INV,  INV,  INV,  INV, /* 0x90..0x97 */
	 INV,  INV,  INV,  INV,  INV,  INV,  INV,  INV, /* 0x98..0x9f */
	 INV,  INV,  INV,  INV,  INV,  INV,  INV,  INV, /* 0xa0..0xa7 */
	 INV,  INV,  INV,  INV,  INV,  INV,  INV,  INV, /* 0xa8..0xaf */
	 INV,  INV,  INV,  INV,  INV,  INV,  INV,  INV, /* 0xb0..0xb7 */
	 INV,  INV,  INV,  INV,  INV,  INV,  INV,  INV, /* 0xb8..0xbf */
	 INV,  INV,  INV,  INV,  INV,  INV,  INV,  INV, /* 0xc0..0xc7 */
	 INV,  INV,  INV,  INV,  INV,  INV,  INV,  INV, /* 0xc8..0xcf */
	 INV,  INV,  INV,  INV,  INV,  INV,  INV,  INV, /* 0xd0..0xd7 */
	 INV,  INV,  INV,  INV,  INV,  INV,  INV,  INV, /* 0xd8..0xdf */
	 INV,  INV,  INV,  INV,  INV,  INV,  INV,  INV, /* 0xe0..0xe7 */
	 INV,  INV,  INV,  INV,  INV,  INV,  INV,  INV, /* 0xe8..0xef */
	 INV,  INV,  INV,  INV,  INV,  INV,  INV,  INV, /* 0xf0..0xf7 */
	 INV,  INV,  INV,  INV,  INV,  INV,  INV,  INV, /* 0xf8..0xff */
};

__attribute__((always_inline))
static inline void __b64_encode(char *out, const void *_in, size_t inlen,
				const char *table)
{
	const uint8_t *in = _in;
	const size_t groups = inlen / 3;
	uint32_t v;
	size_t i;

	for (i = 0; i < groups; i++) {
		const size_t iidx = i * 3;
		const size_t oidx = i * 4;

		v =  in[iidx] << 16;
		v |= in[iidx + 1] << 8;
		v |= in[iidx + 2];

		out[oidx + 0] = table[(v >> 18) & 0x3f];
		out[oidx + 1] = table[(v >> 12) & 0x3f];
		out[oidx + 2] = table[(v >> 6) & 0x3f];
		out[oidx + 3] = table[(v >> 0) & 0x3f];
	}

	switch (inlen % 3) {
		case 0:
			/* nothing left - nul terminate */
			out[i * 4] = '\0';
			break;
		case 1:
			/* 1 byte left - encoded it & pad with == */
			v = in[i * 3];
			v <<= 4;

			out[i * 4]     = table[(v >> 6) & 0x3f];
			out[i * 4 + 1] = table[(v >> 0) & 0x3f];
			out[i * 4 + 2] = '=';
			out[i * 4 + 3] = '=';
			out[i * 4 + 4] = '\0';
			break;
		case 2:
			/* 2 bytes left - encoded them & pad with = */
			v =  in[i * 3] << 8;
			v |= in[i * 3 + 1];
			v <<= 2;

			out[i * 4]     = table[(v >> 12) & 0x3f];
			out[i * 4 + 1] = table[(v >> 6) & 0x3f];
			out[i * 4 + 2] = table[(v >> 0) & 0x3f];
			out[i * 4 + 3] = '=';
			out[i * 4 + 4] = '\0';
			break;
	}
}

void base64_encode(char *out, const void *in, size_t inlen)
{
	__b64_encode(out, in, inlen, b64_encode_table);
}

void base64url_encode(char *out, const void *in, size_t inlen)
{
	__b64_encode(out, in, inlen, b64url_encode_table);
}

__attribute__((always_inline))
static inline ssize_t __b64_decode(void *_out, const char *_in, size_t inlen,
				   const uint8_t *table)
{
	const uint8_t *in = (const uint8_t *) _in;
	uint8_t *out = _out;
	size_t groups;
	size_t i;

	/* special case: empty input means empty output */
	if (!inlen)
		return 0;

	/* must have full groups */
	if (inlen % 4)
		return -1;

	groups = inlen / 4;
	if (in[inlen - 1] == '=')
		groups--; /* last group has padding */

	for (i = 0; i < groups; i++) {
		const uint8_t a = table[in[(i * 4) + 0]];
		const uint8_t b = table[in[(i * 4) + 1]];
		const uint8_t c = table[in[(i * 4) + 2]];
		const uint8_t d = table[in[(i * 4) + 3]];
		uint32_t v;

		if (CHECK_INV(a, b, c, d))
			return -1;

		v = (a << 18) | (b << 12) | (c << 6) | d;

		out[(i * 3) + 0] = (v >> 16) & 0xff;
		out[(i * 3) + 1] = (v >> 8) & 0xff;
		out[(i * 3) + 2] = (v >> 0) & 0xff;
	}

	if ((in[inlen - 2] == '=') && (in[inlen - 1] == '=')) {
		/* two pad chars, 1 byte of output left */
		const uint8_t a = table[in[inlen - 4]];
		const uint8_t b = table[in[inlen - 3]];
		uint32_t v;

		if (CHECK_INV(a, b, 0, 0))
			return -1;

		v = (a << 6) | b;
		v >>= 4;

		out[(groups * 3) + 0] = v & 0xff;

		return (groups * 3) + 1;
	} else if (in[inlen - 1] == '=') {
		/* one pad char, 2 bytes of output left */
		const uint8_t a = table[in[inlen - 4]];
		const uint8_t b = table[in[inlen - 3]];
		const uint8_t c = table[in[inlen - 2]];
		uint32_t v;

		if (CHECK_INV(a, b, c, 0))
			return -1;

		v = (a << 12) | (b << 6) | c;
		v >>= 2;

		out[(groups * 3) + 0] = (v >> 8) & 0xff;
		out[(groups * 3) + 1] = (v >> 0) & 0xff;

		return (groups * 3) + 2;
	} else {
		/* zero pad chars, nothing to do */
		return (groups * 3);
	}
}

ssize_t base64_decode(void *out, const char *in, size_t inlen)
{
	return __b64_decode(out, in, inlen, b64_decode_table);
}

ssize_t base64url_decode(void *out, const char *in, size_t inlen)
{
	return __b64_decode(out, in, inlen, b64url_decode_table);
}
