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

#include <stdbool.h>

#include <jeffpc/error.h>

#include "test.c"

static inline const char *boolstr(bool b)
{
	return b ? "true" : "false";
}

static void do_test(bool err, const void *constptr, void *ptr, int i,
		    const char *e)
{
	fprintf(stderr, "checking %-8s: %p %p %d...", e, constptr, ptr, i);

	if (constptr != ptr)
		fail("input pointers don't match: %p != %p", constptr, ptr);

	if (IS_ERR(constptr) != err)
		fail("const IS_ERR(%p) != expectations (%s != %s)", constptr,
		     boolstr(IS_ERR(constptr)), boolstr(err));

	if (IS_ERR(ptr) != err)
		fail("IS_ERR(%p) != expectations (%s != %s)", ptr,
		     boolstr(IS_ERR(constptr)), boolstr(err));

	if (err) {
		if (PTR_ERR(constptr) != i)
			fail("const PTR_ERR(%p) != expectations (%d != %d)",
			     constptr, PTR_ERR(constptr), i);

		if (PTR_ERR(ptr) != i)
			fail("PTR_ERR(%p) != expectations (%d != %d)",
			     ptr, PTR_ERR(ptr), i);

		if (ERR_PTR(i) != constptr)
			fail("ERR_PTR(%d) != const ptr (%p != %p)", i,
			     ERR_PTR(i), constptr);

		if (ERR_PTR(i) != ptr)
			fail("ERR_PTR(%d) != ptr (%p != %p)", i, ERR_PTR(i),
			     ptr);

		if (ERR_CAST(ptr) != constptr)
			fail("ERR_CAST(%p) != %p (%p != %p)", ptr, constptr,
			     ERR_CAST(ptr), constptr);

		if (ERR_CAST(constptr) != ptr)
			fail("ERR_CAST(%p) != %p (%p != %p)", constptr, ptr,
			     ERR_CAST(constptr), ptr);
	}

	fprintf(stderr, "done.\n");
}

#define TEST(err, e)							\
	do_test((err), (void *)(uintptr_t) (e), (void *)(uintptr_t) (e),\
		(e), #e)

void test(void)
{
				/* illumos/unleashed	linux */
	TEST(true, -EPERM);	/* 1			? */
	TEST(true, -ESTALE);	/* 151			? */

	TEST(false, 0);
	TEST(false, 0xfffff8fful);
}
