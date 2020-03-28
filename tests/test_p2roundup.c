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

#include <jeffpc/types.h>

#include "test.c"

#define CHECK(name, idx, _val, _align, _exp)				\
	do {								\
		const uint64_t v = (_val);				\
		const uint64_t a = (_align);				\
		const uint64_t exp = (_exp);				\
		uint64_t got;						\
									\
		fprintf(stderr, "%s: %2d: p2roundup(%#018"PRIx64", "	\
			"%#018"PRIx64")...", (name), (idx), v, a);	\
									\
		got = p2roundup(v, a);					\
		if (got != exp)						\
			fail("expected %#018"PRIx64", got %#018"PRIx64,	\
			     exp, got);					\
									\
		fprintf(stderr, "ok.\n");				\
	} while (0)

#define TEST(name, _in, out)						\
	do {								\
		const uint64_t val = (_in);				\
		int shift;						\
									\
		/* no alignment -> no change */				\
		CHECK((name), 0, val, 0ul, val);			\
		CHECK((name), 1, val, 1ul, val);			\
									\
		for (shift = 1; shift < 64; shift++) {			\
			const uint64_t align = 1ull << shift;		\
			CHECK((name), shift + 1, val, align, (out));	\
		}							\
	} while (0)

void test(void)
{
	/* aligning zero always yields zero */
	TEST("zero", 0ul, 0);

	/* aligning one always yields alignment */
	TEST("one", 1ul, align);

	/* aligning INT64_MAX yields INT64_MAX+1 */
	TEST("INT64_MAX", INT64_MAX, ((uint64_t) INT64_MAX) + 1);

	/* aligning UINT64_MAX yields zero because of overflow */
	TEST("UINT64_MAX", UINT64_MAX, 0);

	/* check against alternate implementations */
#define TEST_BATCH(name) \
	do { \
		TEST(name " 0x111...", 0x1111111111111111, ALT_IMPL); \
		TEST(name " 0x555...", 0x5555555555555555, ALT_IMPL); \
		TEST(name " 0xaaa...", 0xaaaaaaaaaaaaaaaa, ALT_IMPL); \
		TEST(name " digits asc", 0x0123456789abcdef, ALT_IMPL); \
		TEST(name " digits desc", 0xfedcba9876543210, ALT_IMPL); \
	} while (0)

#define ALT_IMPL (((val >> shift) + ((val << (64 - shift)) ? 1 : 0)) << shift)
	TEST_BATCH("ALT1");
#undef ALT_IMPL
#define ALT_IMPL (val + ((val % align) ? (align - (val % align)) : 0))
	TEST_BATCH("ALT2");
#undef ALT_IMPL
#define ALT_IMPL (val + ((val & (align - 1)) ? (align - (val & (align - 1))) : 0))
	TEST_BATCH("ALT3");
#undef ALT_IMPL
}
