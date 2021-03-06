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
	 * Check min/max for each uint size
	 *
	 * Note: We don't use base prefixes (0 & 0x) in all the inputs to
	 * test those inputs in different bases as well - they are "free"
	 * test cases.
	 */
	/* highest 8-bit ... */
	{
		.in       = "255", /* ... base 10 */
		.out[B8]  = ENT(0255,  0255,  0255,  0255, 0, 0, 0, 0),
		.out[B10] = ENT( 255,   255,   255,   255, 0, 0, 0, 0),
		.out[B16] = ENT(   0, 0x255, 0x255, 0x255, -ERANGE, 0, 0, 0),
	},
	{
		.in       = "377", /* ... base 8 (no prefix) */
		.out[B8]  = ENT(0377,  0377,  0377,  0377, 0, 0, 0, 0),
		.out[B10] = ENT(   0,   377,   377,   377, -ERANGE, 0, 0, 0),
		.out[B16] = ENT(   0, 0x377, 0x377, 0x377, -ERANGE, 0, 0, 0),
	},
	{
		.in       = "0377", /* ... base 8 (prefix) */
		.out[B8]  = ENT(0377,  0377,  0377,  0377, 0, 0, 0, 0),
		.out[B10] = ENT(   0,   377,   377,   377, -ERANGE, 0, 0, 0),
		.out[B16] = ENT(   0, 0x377, 0x377, 0x377, -ERANGE, 0, 0, 0),
	},
	{
		.in       = "ff", /* ... base 16 (no prefix) */
		.out[B8]  = ENT(   0,    0,    0,    0, -EINVAL, -EINVAL, -EINVAL, -EINVAL),
		.out[B10] = ENT(   0,    0,    0,    0, -EINVAL, -EINVAL, -EINVAL, -EINVAL),
		.out[B16] = ENT(0xff, 0xff, 0xff, 0xff, 0, 0, 0, 0),
	},
	{
		.in       = "0xff", /* ... base 16 (prefix) */
		.out[B8]  = ENT(   0,    0,    0,    0, -EINVAL, -EINVAL, -EINVAL, -EINVAL),
		.out[B10] = ENT(   0,    0,    0,    0, -EINVAL, -EINVAL, -EINVAL, -EINVAL),
		.out[B16] = ENT(0xff, 0xff, 0xff, 0xff, 0, 0, 0, 0),
	},
	/* higest 8-bit + 1 ... */
	{
		.in       = "256", /* ... base 10 */
		.out[B8]  = ENT(0256,  0256,  0256,  0256, 0, 0, 0, 0),
		.out[B10] = ENT(   0,   256,   256,   256, -ERANGE, 0, 0, 0),
		.out[B16] = ENT(   0, 0x256, 0x256, 0x256, -ERANGE, 0, 0, 0),
	},
	{
		.in       = "400", /* ... base 8 (no prefix) */
		.out[B8]  = ENT(0,  0400,  0400,  0400, -ERANGE, 0, 0, 0),
		.out[B10] = ENT(0,   400,   400,   400, -ERANGE, 0, 0, 0),
		.out[B16] = ENT(0, 0x400, 0x400, 0x400, -ERANGE, 0, 0, 0),
	},
	{
		.in       = "0400", /* ... base 8 (prefix) */
		.out[B8]  = ENT(0,  0400,  0400,  0400, -ERANGE, 0, 0, 0),
		.out[B10] = ENT(0,   400,   400,   400, -ERANGE, 0, 0, 0),
		.out[B16] = ENT(0, 0x400, 0x400, 0x400, -ERANGE, 0, 0, 0),
	},
	{
		.in       = "100", /* ... base 16 (no prefix) */
		.out[B8]  = ENT(0100,  0100,  0100,  0100, 0, 0, 0, 0),
		.out[B10] = ENT( 100,   100,   100,   100, 0, 0, 0, 0),
		.out[B16] = ENT(   0, 0x100, 0x100, 0x100, -ERANGE, 0, 0, 0),
	},
	{
		.in       = "0x100", /* ... base 16 (prefix) */
		.out[B8]  = ENT(0,     0,     0,     0, -EINVAL, -EINVAL, -EINVAL, -EINVAL),
		.out[B10] = ENT(0,     0,     0,     0, -EINVAL, -EINVAL, -EINVAL, -EINVAL),
		.out[B16] = ENT(0, 0x100, 0x100, 0x100, -ERANGE, 0, 0, 0),
	},
	/* highest 16-bit ... */
	{
		.in       = "65535", /* ... base 10 */
		.out[B8]  = ENT(0, 065535,  065535,  065535, -ERANGE, 0, 0, 0),
		.out[B10] = ENT(0,  65535,   65535,   65535, -ERANGE, 0, 0, 0),
		.out[B16] = ENT(0,      0, 0x65535, 0x65535, -ERANGE, -ERANGE, 0, 0),
	},
	{
		.in       = "177777", /* ... base 8 (no prefix) */
		.out[B8]  = ENT(0, 0177777,  0177777,  0177777, -ERANGE, 0, 0, 0),
		.out[B10] = ENT(0,       0,   177777,   177777, -ERANGE, -ERANGE, 0, 0),
		.out[B16] = ENT(0,       0, 0x177777, 0x177777, -ERANGE, -ERANGE, 0, 0),
	},
	{
		.in       = "0177777", /* ... base 8 (prefix) */
		.out[B8]  = ENT(0, 0177777,  0177777,  0177777, -ERANGE, 0, 0, 0),
		.out[B10] = ENT(0,       0,   177777,   177777, -ERANGE, -ERANGE, 0, 0),
		.out[B16] = ENT(0,       0, 0x177777, 0x177777, -ERANGE, -ERANGE, 0, 0),
	},
	{
		.in       = "ffff", /* ... base 16 (no prefix) */
		.out[B8]  = ENT(0,      0,      0,      0, -EINVAL, -EINVAL, -EINVAL, -EINVAL),
		.out[B10] = ENT(0,      0,      0,      0, -EINVAL, -EINVAL, -EINVAL, -EINVAL),
		.out[B16] = ENT(0, 0xffff, 0xffff, 0xffff, -ERANGE, 0, 0, 0),
	},
	{
		.in       = "0xffff", /* ... base 16 (prefix) */
		.out[B8]  = ENT(0,      0,      0,      0, -EINVAL, -EINVAL, -EINVAL, -EINVAL),
		.out[B10] = ENT(0,      0,      0,      0, -EINVAL, -EINVAL, -EINVAL, -EINVAL),
		.out[B16] = ENT(0, 0xffff, 0xffff, 0xffff, -ERANGE, 0, 0, 0),
	},
	/* highest 16-bit + 1 ... */
	{
		.in       = "65536", /* ... base 10 */
		.out[B8]  = ENT(0, 065536,  065536,  065536, -ERANGE, 0, 0, 0),
		.out[B10] = ENT(0,      0,   65536,   65536, -ERANGE, -ERANGE, 0, 0),
		.out[B16] = ENT(0,      0, 0x65536, 0x65536, -ERANGE, -ERANGE, 0, 0),
	},
	{
		.in       = "200000", /* ... base 8 (no prefix) */
		.out[B8]  = ENT(0, 0,  0200000,  0200000, -ERANGE, -ERANGE, 0, 0),
		.out[B10] = ENT(0, 0,   200000,   200000, -ERANGE, -ERANGE, 0, 0),
		.out[B16] = ENT(0, 0, 0x200000, 0x200000, -ERANGE, -ERANGE, 0, 0),
	},
	{
		.in       = "0200000", /* ... base 8 (prefix) */
		.out[B8]  = ENT(0, 0,  0200000,  0200000, -ERANGE, -ERANGE, 0, 0),
		.out[B10] = ENT(0, 0,   200000,   200000, -ERANGE, -ERANGE, 0, 0),
		.out[B16] = ENT(0, 0, 0x200000, 0x200000, -ERANGE, -ERANGE, 0, 0),
	},
	{
		.in       = "10000", /* ... base 16 (no prefix) */
		.out[B8]  = ENT(0, 010000,  010000,  010000, -ERANGE, 0, 0, 0),
		.out[B10] = ENT(0,  10000,   10000,   10000, -ERANGE, 0, 0, 0),
		.out[B16] = ENT(0,      0, 0x10000, 0x10000, -ERANGE, -ERANGE, 0, 0),
	},
	{
		.in       = "0x10000", /* ... base 16 (prefix) */
		.out[B8]  = ENT(0,      0,      0,      0, -EINVAL, -EINVAL, -EINVAL, -EINVAL),
		.out[B10] = ENT(0,      0,      0,      0, -EINVAL, -EINVAL, -EINVAL, -EINVAL),
		.out[B16] = ENT(0, 0, 0x10000, 0x10000, -ERANGE, -ERANGE, 0, 0),
	},
	/* highest 32-bit ... */
	{
		.in       = "4294967295", /* ... base 10 */
		.out[B8]  = ENT(0, 0,          0,            0, -EINVAL, -EINVAL, -EINVAL, -EINVAL),
		.out[B10] = ENT(0, 0, 4294967295,   4294967295, -ERANGE, -ERANGE, 0, 0),
		.out[B16] = ENT(0, 0,          0, 0x4294967295, -ERANGE, -ERANGE, -ERANGE, 0),
	},
	{
		.in       = "37777777777", /* ... base 8 (no prefix) */
		.out[B8]  = ENT(0, 0, 037777777777,  037777777777, -ERANGE, -ERANGE, 0, 0),
		.out[B10] = ENT(0, 0,            0,   37777777777, -ERANGE, -ERANGE, -ERANGE, 0),
		.out[B16] = ENT(0, 0,            0, 0x37777777777, -ERANGE, -ERANGE, -ERANGE, 0),
	},
	{
		.in       = "037777777777", /* ... base 8 (prefix) */
		.out[B8]  = ENT(0, 0, 037777777777,  037777777777, -ERANGE, -ERANGE, 0, 0),
		.out[B10] = ENT(0, 0,            0,   37777777777, -ERANGE, -ERANGE, -ERANGE, 0),
		.out[B16] = ENT(0, 0,            0, 0x37777777777, -ERANGE, -ERANGE, -ERANGE, 0),
	},
	{
		.in       = "ffffffff", /* ... base 16 (no prefix) */
		.out[B8]  = ENT(0, 0,          0,          0, -EINVAL, -EINVAL, -EINVAL, -EINVAL),
		.out[B10] = ENT(0, 0,          0,          0, -EINVAL, -EINVAL, -EINVAL, -EINVAL),
		.out[B16] = ENT(0, 0, 0xffffffff, 0xffffffff, -ERANGE, -ERANGE, 0, 0),
	},
	{
		.in       = "0xffffffff", /* ... base 16 (prefix) */
		.out[B8]  = ENT(0, 0,          0,          0, -EINVAL, -EINVAL, -EINVAL, -EINVAL),
		.out[B10] = ENT(0, 0,          0,          0, -EINVAL, -EINVAL, -EINVAL, -EINVAL),
		.out[B16] = ENT(0, 0, 0xffffffff, 0xffffffff, -ERANGE, -ERANGE, 0, 0),
	},
	/* highest 32-bit + 1 ... */
	{
		.in       = "4294967296", /* ... base 10 */
		.out[B8]  = ENT(0, 0, 0,            0, -EINVAL, -EINVAL, -EINVAL, -EINVAL),
		.out[B10] = ENT(0, 0, 0,   4294967296, -ERANGE, -ERANGE, -ERANGE, 0),
		.out[B16] = ENT(0, 0, 0, 0x4294967296, -ERANGE, -ERANGE, -ERANGE, 0),
	},
	{
		.in       = "40000000000", /* ... base 8 (no prefix) */
		.out[B8]  = ENT(0, 0, 0,  040000000000, -ERANGE, -ERANGE, -ERANGE, 0),
		.out[B10] = ENT(0, 0, 0,   40000000000, -ERANGE, -ERANGE, -ERANGE, 0),
		.out[B16] = ENT(0, 0, 0, 0x40000000000, -ERANGE, -ERANGE, -ERANGE, 0),
	},
	{
		.in       = "040000000000", /* ... base 8 (prefix) */
		.out[B8]  = ENT(0, 0, 0,  040000000000, -ERANGE, -ERANGE, -ERANGE, 0),
		.out[B10] = ENT(0, 0, 0,   40000000000, -ERANGE, -ERANGE, -ERANGE, 0),
		.out[B16] = ENT(0, 0, 0, 0x40000000000, -ERANGE, -ERANGE, -ERANGE, 0),
	},
	{
		.in       = "100000000", /* ... base 16 (no prefix) */
		.out[B8]  = ENT(0, 0, 0100000000,  0100000000, -ERANGE, -ERANGE, 0, 0),
		.out[B10] = ENT(0, 0,  100000000,   100000000, -ERANGE, -ERANGE, 0, 0),
		.out[B16] = ENT(0, 0,          0, 0x100000000, -ERANGE, -ERANGE, -ERANGE, 0),
	},
	{
		.in       = "0x100000000", /* ... base 16 (prefix) */
		.out[B8]  = ENT(0, 0, 0,           0, -EINVAL, -EINVAL, -EINVAL, -EINVAL),
		.out[B10] = ENT(0, 0, 0,           0, -EINVAL, -EINVAL, -EINVAL, -EINVAL),
		.out[B16] = ENT(0, 0, 0, 0x100000000, -ERANGE, -ERANGE, -ERANGE, 0),
	},
	/* highest 64-bit ... */
	{
		.in       = "18446744073709551615", /* ... base 10 */
		.out[B8]  = ENT(0, 0, 0,                       0, -EINVAL, -EINVAL, -EINVAL, -EINVAL),
		.out[B10] = ENT(0, 0, 0, 18446744073709551615ull, -ERANGE, -ERANGE, -ERANGE, 0),
		.out[B16] = ENT(0, 0, 0,                       0, -ERANGE, -ERANGE, -ERANGE, -ERANGE),
	},
	{
		.in       = "1777777777777777777777", /* ... base 8 (no prefix) */
		.out[B8]  = ENT(0, 0, 0, 01777777777777777777777, -ERANGE, -ERANGE, -ERANGE, 0),
		.out[B10] = ENT(0, 0, 0,                       0, -ERANGE, -ERANGE, -ERANGE, -ERANGE),
		.out[B16] = ENT(0, 0, 0,                       0, -ERANGE, -ERANGE, -ERANGE, -ERANGE),
	},
	{
		.in       = "1777777777777777777777", /* ... base 8 (prefix) */
		.out[B8]  = ENT(0, 0, 0, 01777777777777777777777, -ERANGE, -ERANGE, -ERANGE, 0),
		.out[B10] = ENT(0, 0, 0,                       0, -ERANGE, -ERANGE, -ERANGE, -ERANGE),
		.out[B16] = ENT(0, 0, 0,                       0, -ERANGE, -ERANGE, -ERANGE, -ERANGE),
	},
	{
		.in       = "ffffffffffffffff", /* ... base 16 (no prefix) */
		.out[B8]  = ENT(0, 0, 0,                  0, -EINVAL, -EINVAL, -EINVAL, -EINVAL),
		.out[B10] = ENT(0, 0, 0,                  0, -EINVAL, -EINVAL, -EINVAL, -EINVAL),
		.out[B16] = ENT(0, 0, 0, 0xffffffffffffffff, -ERANGE, -ERANGE, -ERANGE, 0),
	},
	{
		.in       = "0xffffffffffffffff", /* ... base 16 (prefix) */
		.out[B8]  = ENT(0, 0, 0,                  0, -EINVAL, -EINVAL, -EINVAL, -EINVAL),
		.out[B10] = ENT(0, 0, 0,                  0, -EINVAL, -EINVAL, -EINVAL, -EINVAL),
		.out[B16] = ENT(0, 0, 0, 0xffffffffffffffff, -ERANGE, -ERANGE, -ERANGE, 0),
	},
	/* highest 64-bit + 1 ... */
	{
		.in       = "18446744073709551616", /* ... base 10 */
		.out[B8]  = ENT(0, 0, 0, 0, -EINVAL, -EINVAL, -EINVAL, -EINVAL),
		.out[B10] = ENT(0, 0, 0, 0, -ERANGE, -ERANGE, -ERANGE, -ERANGE),
		.out[B16] = ENT(0, 0, 0, 0, -ERANGE, -ERANGE, -ERANGE, -ERANGE),
	},
	{
		.in       = "2000000000000000000000", /* ... base 8 (no prefix) */
		.out[B8]  = ENT(0, 0, 0, 0, -ERANGE, -ERANGE, -ERANGE, -ERANGE),
		.out[B10] = ENT(0, 0, 0, 0, -ERANGE, -ERANGE, -ERANGE, -ERANGE),
		.out[B16] = ENT(0, 0, 0, 0, -ERANGE, -ERANGE, -ERANGE, -ERANGE),
	},
	{
		.in       = "02000000000000000000000", /* ... base 8 (prefix) */
		.out[B8]  = ENT(0, 0, 0, 0, -ERANGE, -ERANGE, -ERANGE, -ERANGE),
		.out[B10] = ENT(0, 0, 0, 0, -ERANGE, -ERANGE, -ERANGE, -ERANGE),
		.out[B16] = ENT(0, 0, 0, 0, -ERANGE, -ERANGE, -ERANGE, -ERANGE),
	},
	{
		.in       = "10000000000000000", /* ... base 16 (no prefix) */
		.out[B8]  = ENT(0, 0, 0, 010000000000000000, -ERANGE, -ERANGE, -ERANGE, 0),
		.out[B10] = ENT(0, 0, 0,  10000000000000000, -ERANGE, -ERANGE, -ERANGE, 0),
		.out[B16] = ENT(0, 0, 0,                  0, -ERANGE, -ERANGE, -ERANGE, -ERANGE),
	},
	{
		.in       = "0x10000000000000000", /* ... base 16 (prefix) */
		.out[B8]  = ENT(0, 0, 0, 0, -EINVAL, -EINVAL, -EINVAL, -EINVAL),
		.out[B10] = ENT(0, 0, 0, 0, -EINVAL, -EINVAL, -EINVAL, -EINVAL),
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
