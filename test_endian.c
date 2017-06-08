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

	uint8_t le8;
	uint16_t le16;
	uint32_t le32;
	uint64_t le64;

#ifdef CPU_BIG_ENDIAN
#define cpu8 be8
#define cpu16 be16
#define cpu32 be32
#define cpu64 be64
#else
#define cpu8 le8
#define cpu16 le16
#define cpu32 le32
#define cpu64 le64
#endif
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
		.le8	= 0,
		.le16	= 0,
		.le32	= 0,
		.le64	= 0,
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
		.le8	= 0xff,
		.le16	= 0xffff,
		.le32	= 0xffffffff,
		.le64	= 0xffffffffffffffff,
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
		.le8	= 0x12,
		.le16	= 0x3412,
		.le32	= 0x78563412,
		.le64	= 0xf0debc9a78563412,
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
		.le8	= 0x80,
		.le16	= 0x8080,
		.le32	= 0x80808080,
		.le64	= 0x8080808080808080,
	},
};

#define __CHECK_READ(iter, size, pfx, fmt, in, fxn, exp)		\
	do {								\
		uint##size##_t got = fxn(in);				\
									\
		fprintf(stderr, "R %d: %-3s expected: %#"fmt"\n", iter,	\
			pfx, exp);					\
		fprintf(stderr, "R %d: %-3s got:      %#"fmt"\n", iter, \
			pfx, got);					\
									\
		if (got != exp)						\
			fail("mismatch!");				\
	} while (0)

#define CHECK_READ(iter, size, fmt, in, be_exp, le_exp, cpu_exp)	\
	do {								\
		__CHECK_READ(iter, size, "BE", fmt, in,			\
			     be##size##_to_cpu_unaligned, be_exp);	\
		__CHECK_READ(iter, size, "LE", fmt, in,			\
			     le##size##_to_cpu_unaligned, le_exp);	\
		__CHECK_READ(iter, size, "CPU", fmt, in,		\
			     cpu##size##_to_cpu_unaligned, cpu_exp);	\
	} while (0)

static void __test_read(int iter, const struct unaligned_run *run)
{
	char dumped[sizeof(run->in) * 2 + 1];

	hexdumpz(dumped, run->in, sizeof(run->in), false);

	fprintf(stderr, "R %d: input:        %s\n", iter, dumped);
	CHECK_READ(iter, 8, PRIx8, run->in, run->be8, run->le8, run->cpu8);
	CHECK_READ(iter, 16, PRIx16, run->in, run->be16, run->le16, run->cpu16);
	CHECK_READ(iter, 32, PRIx32, run->in, run->be32, run->le32, run->cpu32);
	CHECK_READ(iter, 64, PRIx64, run->in, run->be64, run->le64, run->cpu64);
	fprintf(stderr, "R %d: ok.\n", iter);
}

#define __CHECK_WRITE(iter, size, pfx, in, fxn, exp)			\
	do {								\
		char exp_str[2 * size / 8 + 1];				\
		char got_str[2 * size / 8 + 1];				\
		uint8_t got[size / 8];					\
									\
		fxn(in, got);						\
									\
		hexdumpz(exp_str, exp, size / 8, false);		\
		hexdumpz(got_str, got, size / 8, false);		\
									\
		fprintf(stderr, "W %d: %-3s expected: %s\n", iter, pfx,	\
			exp_str);					\
		fprintf(stderr, "W %d: %-3s got:      %s\n", iter, pfx,	\
			got_str);					\
									\
		if (memcmp(got, exp, sizeof(got)))			\
			fail("mismatch!");				\
	} while (0)

#define CHECK_WRITE(iter, size, fmt, out, be_in)			\
	do {								\
		fprintf(stderr, "W %d: input: %#"fmt"\n", iter, be_in);	\
									\
		__CHECK_WRITE(iter, size, "BE", be_in,			\
			      cpu##size##_to_be_unaligned, out);	\
	} while (0)

static void __test_write(int iter, const struct unaligned_run *run)
{
	CHECK_WRITE(iter, 8, PRIx8, run->in, run->be8);
	CHECK_WRITE(iter, 16, PRIx16, run->in, run->be16);
	CHECK_WRITE(iter, 32, PRIx32, run->in, run->be32);
	CHECK_WRITE(iter, 64, PRIx64, run->in, run->be64);
}

void test(void)
{
	int i;

	for (i = 0; i < ARRAY_LEN(uruns) ; i++)
		__test_read(i, &uruns[i]);

	for (i = 0; i < ARRAY_LEN(uruns) ; i++)
		__test_write(i, &uruns[i]);
}
