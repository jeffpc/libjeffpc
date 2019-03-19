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

#include <jeffpc/cbor.h>

#include "test.c"

static const struct test {
	int ret;
	enum val_type type;
} tests[256] = {
#define V1(idx, ret, type)	[idx] = { (ret), (type) }
#define V2(idx, ret, type)	V1((idx),     (ret), (type)), \
				V1((idx) + 1, (ret), (type))
#define V4(idx, ret, type)	V2((idx),     (ret), (type)), \
				V2((idx) + 2, (ret), (type))
#define V8(idx, ret, type)	V4((idx),     (ret), (type)), \
				V4((idx) + 4, (ret), (type))
#define V16(idx, ret, type)	V8((idx),     (ret), (type)), \
				V8((idx) + 8, (ret), (type))
#define V32(idx, ret, type)	V16((idx),     (ret), (type)), \
				V16((idx) + 16, (ret), (type))

	V32(0x00, 0,        VT_INT),	/* CMT_UINT */
	V32(0x20, -ENOTSUP, 0),		/* CMT_NINT */
	V32(0x40, 0,        VT_BLOB),	/* CMT_BYTE */
	V32(0x60, 0,        VT_STR),	/* CMT_TEXT */
	V32(0x80, 0,        VT_ARRAY),	/* CMT_ARRAY */
	V32(0xa0, 0,        VT_NVL),    /* CMT_MAP */
	V32(0xc0, -ENOTSUP, 0),		/* CMT_TAG */

	/* CMT_FLOAT is more complicated */
	V16(0xe0, -ENOTSUP, 0),		/* CMT_FLOAT */
	V4 (0xf0, -ENOTSUP, 0),		/* CMT_FLOAT */
	V2 (0xf4, 0,        VT_BOOL),	/* CMT_FLOAT => bool */
	V1 (0xf6, 0,        VT_NULL),	/* CMT_FLOAT => null */
	V1 (0xf7, -ENOTSUP, 0),		/* CMT_FLOAT */
	V4 (0xf8, -ENOTSUP, 0),		/* CMT_FLOAT */
	V2 (0xfc, -ENOTSUP, 0),		/* CMT_FLOAT */
	V1 (0xfe, -ENOTSUP, 0),		/* CMT_FLOAT */
	V1 (0xff, -EINTR,   0),		/* CMT_FLOAT => break */
};

static void test_zero(void)
{
	enum val_type type;
	struct buffer buf;
	int ret;

	fprintf(stderr, "empty input: expecting -EFAULT...");

	buffer_init_static(&buf, NULL, 0, false);

	ret = cbor_peek_type(&buf, &type);

	check_rets(-EFAULT, ret, "cbor_peek_type()");

	fprintf(stderr, "ok.\n");
}

static void test_one(void)
{
	size_t i;

	for (i = 0; i < ARRAY_LEN(tests); i++) {
		const struct test *run = &tests[i];
		struct buffer buf;
		enum val_type type;
		uint8_t in;
		int ret;

		fprintf(stderr, "%3zu: %02zx expecting ret=%d (%s) type=%s...",
			i, i, run->ret, xstrerror(run->ret),
			val_typename(run->type));

		in = i;

		buffer_init_static(&buf, &in, sizeof(in), false);

		ret = cbor_peek_type(&buf, &type);

		check_rets(run->ret, ret, "cbor_peek_type()");

		if (!ret && (type != run->type))
			fail("got %s", val_typename(type));

		fprintf(stderr, "ok.\n");
	}
}

void test(void)
{
	test_zero();
	test_one();
}
