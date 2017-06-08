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

#include <jeffpc/int.h>
#include <jeffpc/types.h>
#include <jeffpc/hexdump.h>

#include "test.c"

struct unaligned_run {
	uint8_t in[8];

	/* outputs */
	uint8_t be8;
	uint16_t be16;
	uint32_t be32;
	uint64_t be64;
};

static const struct unaligned_run uruns[] = {
	{
		.in	= {
			0x00, 0x00, 0x00, 0x00,
			0x00, 0x00, 0x00, 0x00,
		},
		.be8	= 0,
		.be16	= 0,
		.be32	= 0,
		.be64	= 0,
	},
	{
		.in	= {
			0xff, 0xff, 0xff, 0xff,
			0xff, 0xff, 0xff, 0xff,
		},
		.be8	= 0xff,
		.be16	= 0xffff,
		.be32	= 0xffffffff,
		.be64	= 0xffffffffffffffff,
	},
	{
		.in	= {
			0x12, 0x34, 0x56, 0x78,
			0x9a, 0xbc, 0xde, 0xf0,
		},
		.be8	= 0x12,
		.be16	= 0x1234,
		.be32	= 0x12345678,
		.be64	= 0x123456789abcdef0,
	},
	{
		.in	= {
			0x80, 0x80, 0x80, 0x80,
			0x80, 0x80, 0x80, 0x80,
		},
		.be8	= 0x80,
		.be16	= 0x8080,
		.be32	= 0x80808080,
		.be64	= 0x8080808080808080,
	},
};

#define __CHECK(iter, size, pfx, fmt, in, fxn, exp)			\
	do {								\
		uint##size##_t got = fxn(in);				\
									\
		fprintf(stderr, "%d: %-3s expected: %#"fmt"\n", iter, pfx, exp);\
		fprintf(stderr, "%d: %-3s got:      %#"fmt"\n", iter, pfx, got);\
									\
		if (got != exp)						\
			fail("mismatch!");				\
	} while (0)

#define CHECK(iter, size, fmt, in, be_exp)				\
	do {								\
		__CHECK(iter, size, "BE", fmt, in, be##size##_to_cpu_unaligned, be_exp);\
	} while (0)

static void __test(int iter, const struct unaligned_run *run)
{
	char dumped[sizeof(run->in) * 2 + 1];

	hexdumpz(dumped, run->in, sizeof(run->in), false);

	fprintf(stderr, "%d: input:        %s\n", iter, dumped);
	CHECK(iter, 8, PRIx8, run->in, run->be8);
	CHECK(iter, 16, PRIx16, run->in, run->be16);
	CHECK(iter, 32, PRIx32, run->in, run->be32);
	CHECK(iter, 64, PRIx64, run->in, run->be64);
	fprintf(stderr, "%d: ok.\n", iter);
}

void test(void)
{
	int i;

	for (i = 0; i < ARRAY_LEN(uruns) ; i++)
		__test(i, &uruns[i]);
}
