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

#include <stdbool.h>

#include <jeffpc/error.h>

#include "test.c"

/*
 * The callback can either return a string (which gets propagated to the
 * xstrerror caller) or return NULL (which lets generic xstrerror code
 * handle the errno).  We want to test both behaviors.  If this variable is
 * set to true, our callback function returns NULL.  Otherwise, it returns a
 * mangled int (see below).
 */
static bool return_null;

/*
 * Essentially, we want to make it obvious if the test failed because
 * xstrerror returned:
 *
 *  - NULL (bad)
 *  - a negated errno (bad)
 *  - a real string (bad)
 *  - a real-looking pointer (good)
 *
 * The easiest way to do that is to "reserve" a unique pointer for each
 * possible return string.
 */
static char error_ptrs[MAX_ERRNO + 1];

static inline void *mangle_int(int i)
{
	ASSERT3S(i, <=, 0);
	ASSERT3S(i, >=, -MAX_ERRNO);

	return &error_ptrs[-i];
}

static inline bool is_mangled(const char *ptr)
{
	return (ptr >= &error_ptrs[0]) && (ptr <= &error_ptrs[MAX_ERRNO]);
}

static const char *mystrerror(int e)
{
	fprintf(stderr, "called with %d...", e);

	return return_null ? NULL : mangle_int(e);
}

static void __check_err(int e, const char *got)
{
	if (return_null) {
		if (got == NULL)
			fail("expected a string, got NULL");
		if (IS_ERR(got))
			fail("expected a string, got errno %d (%p)",
			     PTR_ERR(got), got);
		if (is_mangled(got))
			fail("expected a string, got mangled ptr");

		/*
		 * It'd be nice if we could check that the string is sane,
		 * but we don't know what exact string to expect.
		 */
	} else {
		void *exp = mangle_int(e);

		if (got != exp)
			fail("expected %p, got %p", exp, got);
	}
}

static void __check_ok(int e, const char *got)
{
	if (return_null) {
		if (got == NULL)
			fail("expected 'Success', got NULL");
		if (IS_ERR(got))
			fail("expected 'Success', got errno %d (%p)",
			     PTR_ERR(got), got);
		if (strcmp(got, "Success"))
			fail("expected 'Success', got '%s'", got);
	} else {
		void *exp = mangle_int(0);

		if (got != exp)
			fail("expected %p, got %p", exp, got);
	}
}

static void __test(int e, void (*check)(int, const char *))
{
	const char *got;

	fprintf(stderr, "%5d %s...", e, return_null ? "null" : "mangled");

	got = xstrerror(e);
	fprintf(stderr, "returned %p...", got);

	check(e, got);

	fprintf(stderr, "ok.\n");
}

static void do_test(void)
{
	int i;

	/* test errors */
	for (i = -MAX_ERRNO + 1; i < 0; i++)
		__test(i, __check_err);

	/* test success */
	__test(0, __check_ok);
}

void test(void)
{
	struct jeffpc_ops init_ops = {
		.strerror = mystrerror,
	};

	/* override the ops set by the generic code */
	jeffpc_init(&init_ops);

	return_null = false;
	do_test();

	return_null = true;
	do_test();
}
