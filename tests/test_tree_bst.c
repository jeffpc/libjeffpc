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

#define TEST_TREE_BST

#include "test_tree_common.c"

static struct test2 checks2[2] = {
	[true]  = {
		"ascending",
		{ &a, NULL, &b },
	},
	[false] = {
		"descending",
		{ &b, &a, NULL },
	},
};

static struct test3 checks3[6] = {
	[0] = {
		.pre = { &a,
			 NULL, &b,
			 NULL, NULL, NULL, &c },
	},
	[1] = {
		.pre = { &a,
			 NULL, &c,
			 NULL, NULL, &b, NULL },
	},
	[2] = {
		.pre = { &b,
			 &a, &c },
	},
	[3] = {
		.pre = { &b,
			 &a, &c },
	},
	[4] = {
		.pre = { &c,
			 &a, NULL,
			 NULL, &b, NULL, NULL },
	},
	[5] = {
		.pre = { &c,
			 &b, NULL,
			 &a, NULL, NULL, NULL },
	},
};

static struct test4 checks4[24] = {
	[0]  = {
		.pre = { &a,
			 NULL, &b,
			 NULL, NULL, NULL, &c,
			 NULL, NULL, NULL, NULL, NULL, NULL, NULL, &d },
	},
	[1]  = {
		.pre = { &a,
			 NULL, &c,
			 NULL, NULL, &b, &d },
	},
	[2]  = {
		.pre = TEST4_BACD,
	},
	[3]  = {
		.pre = TEST4_BACD,
	},
	[4]  = {
		.pre = TEST4_CADB,
	},
	[5]  = {
		.pre = TEST4_CBDA,
	},

	[6]  = {
		.pre = { &a,
			 NULL, &b,
			 NULL, NULL, NULL, &d,
			 NULL, NULL, NULL, NULL, NULL, NULL, &c, NULL },
	},
	[7]  = {
		.pre = { &a,
			 NULL, &c,
			 NULL, NULL, &b, &d },
	},
	[8]  = {
		.pre = TEST4_BADC,
	},
	[9]  = {
		.pre = TEST4_BACD,
	},
	[10] = {
		.pre = TEST4_CADB,
	},
	[11] = {
		.pre = TEST4_CBDA,
	},

	[12] = {
		.pre = { &a,
			 NULL, &d,
			 NULL, NULL, &b, NULL,
			 NULL, NULL, NULL, NULL, NULL, &c, NULL, NULL },
	},
	[13] = {
		.pre = { &a,
			 NULL, &d,
			 NULL, NULL, &c, NULL,
			 NULL, NULL, NULL, NULL, &b, NULL, NULL, NULL },
	},
	[14] = {
		.pre = TEST4_BADC,
	},
	[15] = {
		.pre = TEST4_BADC,
	},
	[16] = {
		.pre = TEST4_CADB,
	},
	[17] = {
		.pre = TEST4_CBDA,
	},

	[18] = {
		.pre = { &d,
			 &a, NULL,
			 NULL, &b, NULL, NULL,
			 NULL, NULL, NULL, &c, NULL, NULL, NULL, NULL },
	},
	[19] = {
		.pre = { &d,
			 &a, NULL,
			 NULL, &c, NULL, NULL,
			 NULL, NULL, &b, NULL, NULL, NULL, NULL, NULL },
	},
	[20] = {
		.pre = { &d,
			 &b, NULL,
			 &a, &c, NULL, NULL},
	},
	[21] = {
		.pre = { &d,
			 &b, NULL,
			 &a, &c, NULL, NULL},
	},
	[22] = {
		.pre = { &d,
			 &c, NULL,
			 &a, NULL, NULL, NULL,
			 NULL, &b, NULL, NULL, NULL, NULL, NULL, NULL },
	},
	[23] = {
		.pre = { &d,
			 &c, NULL,
			 &b, NULL, NULL, NULL,
			 &a, NULL, NULL, NULL, NULL, NULL, NULL, NULL },
	},
};
