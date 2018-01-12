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

#define TEST_TREE_RB

#include "test_tree_common.c"

static struct test2 checks2[2] = {
	[true]  = {
		"ascending",
		{ &a, NULL, &b },
		{
			{ &a, { &b } },
			{ &b, { &a } },
		},
	},
	[false] = {
		"descending",
		{ &b, &a, NULL },
		{
			{ &a, { &b } },
			{ &b, { &a } },
		},
	},
};

/*
 * Since all permutations create the same tree, we test removal only for one
 * of them
 */
static struct test3 checks3[6] = {
	[0] = {
		.pre = { &b, &a, &c },
		.sub = {
			[0] = { &a, { &b, NULL, &c }, },
			[1] = { &b, { &c, &a, NULL }, },
			[2] = { &c, { &b, &a, NULL }, },
		},
	},
	[1] = {
		.pre = { &b, &a, &c },
	},
	[2] = {
		.pre = { &b, &a, &c },
	},
	[3] = {
		.pre = { &b, &a, &c },
	},
	[4] = {
		.pre = { &b, &a, &c },
	},
	[5] = {
		.pre = { &b, &a, &c },
	},
};

/*
 * Since all permutations create only four different trees, we test removal
 * only once for each of them.
 */
static struct test4 checks4[24] = {
	[0]  = {
		.pre = TEST4_BACD,
		.sub = {
			[0] = { &a, { &c, &b, &d }, },
			[1] = { &b, { &c, &a, &d }, },
			[2] = { &c, { &b, &a, &d }, },
			[3] = { &d, { &b, &a, &c }, },
		},
	},
	[1]  = {
		.pre = TEST4_BACD,
	},
	[2]  = {
		.pre = TEST4_BACD,
	},
	[3]  = {
		.pre = TEST4_BACD,
	},
	[4]  = {
		.pre = TEST4_BACD,
	},
	[5]  = {
		.pre = TEST4_BACD,
	},

	[6]  = {
		.pre = TEST4_BADC,
		.sub = {
			[0] = { &a, { &c, &b, &d }, },
			[1] = { &b, { &c, &a, &d }, },
			[2] = { &c, { &b, &a, &d }, },
			[3] = { &d, { &b, &a, &c }, },
		},
	},
	[7]  = {
		.pre = TEST4_CADB,
		.sub = {
			[0] = { &a, { &c, &b, &d }, },
			[1] = { &b, { &c, &a, &d }, },
			[2] = { &c, { &b, &a, &d }, },
			[3] = { &d, { &b, &a, &c }, },
		},
	},
	[8]  = {
		.pre = TEST4_BADC,
	},
	[9]  = {
		.pre = TEST4_CBDA,
		.sub = {
			[0] = { &a, { &c, &b, &d }, },
			[1] = { &b, { &c, &a, &d }, },
			[2] = { &c, { &b, &a, &d }, },
			[3] = { &d, { &b, &a, &c }, },
		},
	},
	[10] = {
		.pre = TEST4_CADB,
	},
	[11] = {
		.pre = TEST4_CBDA,
	},

	[12] = {
		.pre = TEST4_BADC,
	},
	[13] = {
		.pre = TEST4_CADB,
	},
	[14] = {
		.pre = TEST4_BADC,
	},
	[15] = {
		.pre = TEST4_CBDA,
	},
	[16] = {
		.pre = TEST4_CADB,
	},
	[17] = {
		.pre = TEST4_CBDA,
	},

	[18] = {
		.pre = TEST4_BADC,
	},
	[19] = {
		.pre = TEST4_CADB,
	},
	[20] = {
		.pre = TEST4_BADC,
	},
	[21] = {
		.pre = TEST4_CBDA,
	},
	[22] = {
		.pre = TEST4_CADB,
	},
	[23] = {
		.pre = TEST4_CBDA,
	},
};
