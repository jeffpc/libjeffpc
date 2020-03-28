/*
 * Copyright (c) 2020 Josef 'Jeff' Sipek <jeffpc@josefsipek.net>
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

#include "test.c"

#define CHECK(name, idx, _val, exp)					\
	do {								\
		const uint64_t v = (_val);				\
									\
		fprintf(stderr, "%s: %2d: is_p2(%#018"PRIx64")...",	\
			(name), (idx), v);				\
									\
		if (is_p2(v) != (exp))					\
			fail("expected %s, got %s",			\
			     (exp) ? "true" : "false",			\
			     (exp) ? "false" : "true");			\
									\
		fprintf(stderr, "ok.\n");				\
	} while (0)

static void test_powers(void)
{
	int i;

	CHECK("powers", 0, 0ul, true);

	for (i = 0; i < 64; i++)
		CHECK("powers", i + 1, 1ull << i, true);
}

static void test_nonpowers(void)
{
	int i;

	/*
	 * There are many non-powers, so we cannot test them all.
	 * Therefore, we check only a subset.
	 */

	/* all powers of two + 1 */
	for (i = 0; i < 64; i++) {
		const uint64_t val = (1ull << i) + 1;

		CHECK("p2+1", i + 1, val, val <= 2);
	}

	/* all powers of two - 1 */
	for (i = 0; i < 64; i++) {
		const uint64_t val = (1ull << i) - 1;

		CHECK("p2-1", i + 1, val, val <= 2);
	}
}

void test(void)
{
	test_powers();
	test_nonpowers();
}
