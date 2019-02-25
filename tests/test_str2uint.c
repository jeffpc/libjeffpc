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

#include <jeffpc/int.h>
#include <jeffpc/types.h>
#include <jeffpc/error.h>

#include "test.c"

struct res {
	int ret;
	uint64_t out;
};

struct run {
	const char *in;
	struct res out[3][4]; /* out[base][size] */
};

#define B8	0
#define B10	1
#define B16	2

static const int bases[] = {
	[B8] = 8,
	[B10] = 10,
	[B16] = 16,
};

#define SZ8	0
#define SZ16	1
#define SZ32	2
#define SZ64	3

static const int sizes[] = {
	[SZ8]  = 8,
	[SZ16] = 16,
	[SZ32] = 32,
	[SZ64] = 64,
};

#define ENT(v_s8, v_s16, v_s32, v_s64, r_s8, r_s16, r_s32, r_s64) \
	{ \
		[SZ8]  = { .ret = (r_s8),  .out = (v_s8), }, \
		[SZ16] = { .ret = (r_s16), .out = (v_s16), }, \
		[SZ32] = { .ret = (r_s32), .out = (v_s32), }, \
		[SZ64] = { .ret = (r_s64), .out = (v_s64), }, \
	}

static const struct run runs[] = {
	{
		.in	  = "",
		.out[B8]  = ENT(0, 0, 0, 0, -EINVAL, -EINVAL, -EINVAL, -EINVAL),
		.out[B10] = ENT(0, 0, 0, 0, -EINVAL, -EINVAL, -EINVAL, -EINVAL),
		.out[B16] = ENT(0, 0, 0, 0, -EINVAL, -EINVAL, -EINVAL, -EINVAL),
	},
	/*
	 * Check various well-formed (in at least one base) inputs
	 */
	{
		.in	  = "0",
		.out[B8]  = ENT(0, 0, 0, 0, 0, 0, 0, 0),
		.out[B10] = ENT(0, 0, 0, 0, 0, 0, 0, 0),
		.out[B16] = ENT(0, 0, 0, 0, 0, 0, 0, 0),
	},
	{
		.in	  = "00",
		.out[B8]  = ENT(0, 0, 0, 0, 0, 0, 0, 0),
		.out[B10] = ENT(0, 0, 0, 0, 0, 0, 0, 0),
		.out[B16] = ENT(0, 0, 0, 0, 0, 0, 0, 0),
	},
	{
		.in	  = "0x0",
		.out[B8]  = ENT(0, 0, 0, 0, -EINVAL, -EINVAL, -EINVAL, -EINVAL),
		.out[B10] = ENT(0, 0, 0, 0, -EINVAL, -EINVAL, -EINVAL, -EINVAL),
		.out[B16] = ENT(0, 0, 0, 0, 0, 0, 0, 0),
	},
	{
		.in	  = "5",
		.out[B8]  = ENT(5, 5, 5, 5, 0, 0, 0, 0),
		.out[B10] = ENT(5, 5, 5, 5, 0, 0, 0, 0),
		.out[B16] = ENT(5, 5, 5, 5, 0, 0, 0, 0),
	},
	{
		.in	  = "8",
		.out[B8]  = ENT(0, 0, 0, 0, -EINVAL, -EINVAL, -EINVAL, -EINVAL),
		.out[B10] = ENT(8, 8, 8, 8, 0, 0, 0, 0),
		.out[B16] = ENT(8, 8, 8, 8, 0, 0, 0, 0),
	},
	{
		.in	  = "0x8",
		.out[B8]  = ENT(0, 0, 0, 0, -EINVAL, -EINVAL, -EINVAL, -EINVAL),
		.out[B10] = ENT(0, 0, 0, 0, -EINVAL, -EINVAL, -EINVAL, -EINVAL),
		.out[B16] = ENT(8, 8, 8, 8, 0, 0, 0, 0),
	},
	{
		.in	  = "A",
		.out[B8]  = ENT(0, 0, 0, 0, -EINVAL, -EINVAL, -EINVAL, -EINVAL),
		.out[B10] = ENT(0, 0, 0, 0, -EINVAL, -EINVAL, -EINVAL, -EINVAL),
		.out[B16] = ENT(0xa, 0xa, 0xa, 0xa, 0, 0, 0, 0),
	},
	{
		.in	  = "a",
		.out[B8]  = ENT(0, 0, 0, 0, -EINVAL, -EINVAL, -EINVAL, -EINVAL),
		.out[B10] = ENT(0, 0, 0, 0, -EINVAL, -EINVAL, -EINVAL, -EINVAL),
		.out[B16] = ENT(0xa, 0xa, 0xa, 0xa, 0, 0, 0, 0),
	},
	{
		.in	  = "0XA",
		.out[B8]  = ENT(0, 0, 0, 0, -EINVAL, -EINVAL, -EINVAL, -EINVAL),
		.out[B10] = ENT(0, 0, 0, 0, -EINVAL, -EINVAL, -EINVAL, -EINVAL),
		.out[B16] = ENT(0xa, 0xa, 0xa, 0xa, 0, 0, 0, 0),
	},
	{
		.in	  = "0Xa",
		.out[B8]  = ENT(0, 0, 0, 0, -EINVAL, -EINVAL, -EINVAL, -EINVAL),
		.out[B10] = ENT(0, 0, 0, 0, -EINVAL, -EINVAL, -EINVAL, -EINVAL),
		.out[B16] = ENT(0xa, 0xa, 0xa, 0xa, 0, 0, 0, 0),
	},
	{
		.in	  = "0xA",
		.out[B8]  = ENT(0, 0, 0, 0, -EINVAL, -EINVAL, -EINVAL, -EINVAL),
		.out[B10] = ENT(0, 0, 0, 0, -EINVAL, -EINVAL, -EINVAL, -EINVAL),
		.out[B16] = ENT(0xa, 0xa, 0xa, 0xa, 0, 0, 0, 0),
	},
	{
		.in	  = "0xa",
		.out[B8]  = ENT(0, 0, 0, 0, -EINVAL, -EINVAL, -EINVAL, -EINVAL),
		.out[B10] = ENT(0, 0, 0, 0, -EINVAL, -EINVAL, -EINVAL, -EINVAL),
		.out[B16] = ENT(0xa, 0xa, 0xa, 0xa, 0, 0, 0, 0),
	},
	/*
	 * Check various well-formed inputs for overflows
	 */
	{
		.in	  = "7",
		.out[B8]  = ENT(7, 7, 7, 7, 0, 0, 0, 0),
		.out[B10] = ENT(7, 7, 7, 7, 0, 0, 0, 0),
		.out[B16] = ENT(7, 7, 7, 7, 0, 0, 0, 0),
	},
	{
		.in	  = "70",
		.out[B8]  = ENT(070,  070,  070,  070,  0, 0, 0, 0),
		.out[B10] = ENT(70,   70,   70,   70,   0, 0, 0, 0),
		.out[B16] = ENT(0x70, 0x70, 0x70, 0x70, 0, 0, 0, 0),
	},
	{
		.in	  = "7070",
		.out[B8]  = ENT(0, 07070,  07070,  07070,  -ERANGE, 0, 0, 0),
		.out[B10] = ENT(0, 7070,   7070,   7070,   -ERANGE, 0, 0, 0),
		.out[B16] = ENT(0, 0x7070, 0x7070, 0x7070, -ERANGE, 0, 0, 0),
	},
	{
		.in	  = "70707",
		.out[B8]  = ENT(0, 070707,  070707,  070707, -ERANGE, 0, 0, 0),
		.out[B10] = ENT(0,      0,   70707,   70707, -ERANGE, -ERANGE, 0, 0),
		.out[B16] = ENT(0,      0, 0x70707, 0x70707, -ERANGE, -ERANGE, 0, 0),
	},
	{
		.in	  = "707070",
		.out[B8]  = ENT(0, 0,  0707070,  0707070, -ERANGE, -ERANGE, 0, 0),
		.out[B10] = ENT(0, 0,   707070,   707070, -ERANGE, -ERANGE, 0, 0),
		.out[B16] = ENT(0, 0, 0x707070, 0x707070, -ERANGE, -ERANGE, 0, 0),
	},
	{
		.in	  = "70707070",
		.out[B8]  = ENT(0, 0,  070707070,  070707070, -ERANGE, -ERANGE, 0, 0),
		.out[B10] = ENT(0, 0,   70707070,   70707070, -ERANGE, -ERANGE, 0, 0),
		.out[B16] = ENT(0, 0, 0x70707070, 0x70707070, -ERANGE, -ERANGE, 0, 0),
	},
	{
		.in	  = "7070707070",
		.out[B8]  = ENT(0, 0,  07070707070,  07070707070, -ERANGE, -ERANGE, 0, 0),
		.out[B10] = ENT(0, 0,            0,   7070707070, -ERANGE, -ERANGE, -ERANGE, 0),
		.out[B16] = ENT(0, 0,            0, 0x7070707070, -ERANGE, -ERANGE, -ERANGE, 0),
	},
	{
		.in	  = "707070707070",
		.out[B8]  = ENT(0, 0, 0,  0707070707070, -ERANGE, -ERANGE, -ERANGE, 0),
		.out[B10] = ENT(0, 0, 0,   707070707070, -ERANGE, -ERANGE, -ERANGE, 0),
		.out[B16] = ENT(0, 0, 0, 0x707070707070, -ERANGE, -ERANGE, -ERANGE, 0),
	},
	{
		.in	  = "70707070707070",
		.out[B8]  = ENT(0, 0, 0,  070707070707070, -ERANGE, -ERANGE, -ERANGE, 0),
		.out[B10] = ENT(0, 0, 0,   70707070707070, -ERANGE, -ERANGE, -ERANGE, 0),
		.out[B16] = ENT(0, 0, 0, 0x70707070707070, -ERANGE, -ERANGE, -ERANGE, 0),
	},
	{
		.in	  = "7070707070707070",
		.out[B8]  = ENT(0, 0, 0,  07070707070707070, -ERANGE, -ERANGE, -ERANGE, 0),
		.out[B10] = ENT(0, 0, 0,   7070707070707070, -ERANGE, -ERANGE, -ERANGE, 0),
		.out[B16] = ENT(0, 0, 0, 0x7070707070707070, -ERANGE, -ERANGE, -ERANGE, 0),
	},
	{
		.in	  = "707070707070707070",
		.out[B8]  = ENT(0, 0, 0, 0707070707070707070, -ERANGE, -ERANGE, -ERANGE, 0),
		.out[B10] = ENT(0, 0, 0,  707070707070707070, -ERANGE, -ERANGE, -ERANGE, 0),
		.out[B16] = ENT(0, 0, 0,                   0, -ERANGE, -ERANGE, -ERANGE, -ERANGE),
	},
	{
		.in	  = "70707070707070707070",
		.out[B8]  = ENT(0, 0, 0, 070707070707070707070, -ERANGE, -ERANGE, -ERANGE, 0),
		.out[B10] = ENT(0, 0, 0,                     0, -ERANGE, -ERANGE, -ERANGE, -ERANGE),
		.out[B16] = ENT(0, 0, 0,                     0, -ERANGE, -ERANGE, -ERANGE, -ERANGE),
	},
	{
		.in	  = "707070707070707070707",
		.out[B8]  = ENT(0, 0, 0, 0707070707070707070707, -ERANGE, -ERANGE, -ERANGE, 0),
		.out[B10] = ENT(0, 0, 0,                      0, -ERANGE, -ERANGE, -ERANGE, -ERANGE),
		.out[B16] = ENT(0, 0, 0,                      0, -ERANGE, -ERANGE, -ERANGE, -ERANGE),
	},
	{
		.in	  = "7070707070707070707070",
		.out[B8]  = ENT(0, 0, 0, 0, -ERANGE, -ERANGE, -ERANGE, -ERANGE),
		.out[B10] = ENT(0, 0, 0, 0, -ERANGE, -ERANGE, -ERANGE, -ERANGE),
		.out[B16] = ENT(0, 0, 0, 0, -ERANGE, -ERANGE, -ERANGE, -ERANGE),
	},
	/*
	 * Check negative numbers
	 */
	{
		.in       = "-0",
		.out[B8]  = ENT(0, 0, 0, 0, -ERANGE, -ERANGE, -ERANGE, -ERANGE),
		.out[B10] = ENT(0, 0, 0, 0, -ERANGE, -ERANGE, -ERANGE, -ERANGE),
		.out[B16] = ENT(0, 0, 0, 0, -ERANGE, -ERANGE, -ERANGE, -ERANGE),
	},
	{
		.in       = "-1",
		.out[B8]  = ENT(0, 0, 0, 0, -ERANGE, -ERANGE, -ERANGE, -ERANGE),
		.out[B10] = ENT(0, 0, 0, 0, -ERANGE, -ERANGE, -ERANGE, -ERANGE),
		.out[B16] = ENT(0, 0, 0, 0, -ERANGE, -ERANGE, -ERANGE, -ERANGE),
	},
	{
		.in       = "-8",
		.out[B8]  = ENT(0, 0, 0, 0, -ERANGE, -ERANGE, -ERANGE, -ERANGE),
		.out[B10] = ENT(0, 0, 0, 0, -ERANGE, -ERANGE, -ERANGE, -ERANGE),
		.out[B16] = ENT(0, 0, 0, 0, -ERANGE, -ERANGE, -ERANGE, -ERANGE),
	},
	{
		.in	  = "-A",
		.out[B8]  = ENT(0, 0, 0, 0, -ERANGE, -ERANGE, -ERANGE, -ERANGE),
		.out[B10] = ENT(0, 0, 0, 0, -ERANGE, -ERANGE, -ERANGE, -ERANGE),
		.out[B16] = ENT(0, 0, 0, 0, -ERANGE, -ERANGE, -ERANGE, -ERANGE),
	},
	{
		.in	  = "-a",
		.out[B8]  = ENT(0, 0, 0, 0, -ERANGE, -ERANGE, -ERANGE, -ERANGE),
		.out[B10] = ENT(0, 0, 0, 0, -ERANGE, -ERANGE, -ERANGE, -ERANGE),
		.out[B16] = ENT(0, 0, 0, 0, -ERANGE, -ERANGE, -ERANGE, -ERANGE),
	},
	{
		.in	  = "-0XA",
		.out[B8]  = ENT(0, 0, 0, 0, -ERANGE, -ERANGE, -ERANGE, -ERANGE),
		.out[B10] = ENT(0, 0, 0, 0, -ERANGE, -ERANGE, -ERANGE, -ERANGE),
		.out[B16] = ENT(0, 0, 0, 0, -ERANGE, -ERANGE, -ERANGE, -ERANGE),
	},
	{
		.in	  = "-0Xa",
		.out[B8]  = ENT(0, 0, 0, 0, -ERANGE, -ERANGE, -ERANGE, -ERANGE),
		.out[B10] = ENT(0, 0, 0, 0, -ERANGE, -ERANGE, -ERANGE, -ERANGE),
		.out[B16] = ENT(0, 0, 0, 0, -ERANGE, -ERANGE, -ERANGE, -ERANGE),
	},
	{
		.in	  = "-0xA",
		.out[B8]  = ENT(0, 0, 0, 0, -ERANGE, -ERANGE, -ERANGE, -ERANGE),
		.out[B10] = ENT(0, 0, 0, 0, -ERANGE, -ERANGE, -ERANGE, -ERANGE),
		.out[B16] = ENT(0, 0, 0, 0, -ERANGE, -ERANGE, -ERANGE, -ERANGE),
	},
	{
		.in	  = "-0xa",
		.out[B8]  = ENT(0, 0, 0, 0, -ERANGE, -ERANGE, -ERANGE, -ERANGE),
		.out[B10] = ENT(0, 0, 0, 0, -ERANGE, -ERANGE, -ERANGE, -ERANGE),
		.out[B16] = ENT(0, 0, 0, 0, -ERANGE, -ERANGE, -ERANGE, -ERANGE),
	},
	/*
	 * FIXME: more test cases
	 *   - check inputs with leading garbage
	 *   - check inputs with trailing garbage
	 */
};

#define TEST(i, bidx, szidx, expr, type)				\
	do {								\
		const char *in = runs[i].in;				\
		const struct res *res = &runs[i].out[bidx][szidx];	\
		const int base = bases[bidx];				\
		const int size = sizes[szidx];				\
		type tmp;						\
		uint64_t out;						\
		int ret;						\
									\
		fprintf(stderr, "%2d: ... %s -> ", (i), #expr);		\
									\
		ret = (expr);						\
		out = tmp;						\
									\
		/* print the result we got */				\
		if (ret) {						\
			fprintf(stderr, "ret=%d", ret);			\
		} else {						\
			static const char *fmts[] = {			\
				[B8] = "%#0*"PRIo64,			\
				[B10] = "%#0*"PRIu64,			\
				[B16] = "%#0*"PRIx64,			\
			};						\
			int width = 2 * size / 8;			\
									\
			if (base == 8)					\
				width++; /* extra leading 0 */		\
			else if (base == 16)				\
				width += 2; /* leading 0x */		\
									\
			fprintf(stderr, "out=");			\
			fprintf(stderr, fmts[bidx], width, out);	\
		}							\
		fprintf(stderr, "...");					\
									\
		check_rets(res->ret, ret, "str2u%d", size);		\
									\
		if (!ret)						\
			ASSERT3U(out, ==, res->out);			\
									\
		fprintf(stderr, "ok.\n");				\
	} while (0)

void test(void)
{
	int i;

	for (i = 0; i < ARRAY_LEN(runs); i++) {
		fprintf(stderr, "%2d: in='%s'...\n", i, runs[i].in);

		/* test simple base 10 parsing */
		TEST(i, B10, SZ8, str2u8(in, &tmp), uint8_t);
		TEST(i, B10, SZ16, str2u16(in, &tmp), uint16_t);
		TEST(i, B10, SZ32, str2u32(in, &tmp), uint32_t);
		TEST(i, B10, SZ64, str2u64(in, &tmp), uint64_t);

		/* test underlying function with all bases */
		TEST(i, B8, SZ8, str2u8_base(in, &tmp, 8), uint8_t);
		TEST(i, B8, SZ16, str2u16_base(in, &tmp, 8), uint16_t);
		TEST(i, B8, SZ32, str2u32_base(in, &tmp, 8), uint32_t);
		TEST(i, B8, SZ64, str2u64_base(in, &tmp, 8), uint64_t);

		TEST(i, B10, SZ8, str2u8_base(in, &tmp, 10), uint8_t);
		TEST(i, B10, SZ16, str2u16_base(in, &tmp, 10), uint16_t);
		TEST(i, B10, SZ32, str2u32_base(in, &tmp, 10), uint32_t);
		TEST(i, B10, SZ64, str2u64_base(in, &tmp, 10), uint64_t);

		TEST(i, B16, SZ8, str2u8_base(in, &tmp, 16), uint8_t);
		TEST(i, B16, SZ16, str2u16_base(in, &tmp, 16), uint16_t);
		TEST(i, B16, SZ32, str2u32_base(in, &tmp, 16), uint32_t);
		TEST(i, B16, SZ64, str2u64_base(in, &tmp, 16), uint64_t);
	}
}
