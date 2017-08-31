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

#include <jeffpc/types.h>
#include <jeffpc/sexpr.h>

#include "test.c"

static inline struct val *safe_dump(struct val *val)
{
	int ret;

	ret = sexpr_dump_file(stderr, val, false);
	if (ret)
		fail("%s: failed to dump value: %s", xstrerror(ret));

	return val;
}

static inline void check_loop_body(struct val *got, struct val **exp_items,
				   size_t iter, size_t exp_nitems)
{
	const size_t got_nitems = iter + 1;
	struct val *exp;

	if (got_nitems > exp_nitems)
		fail("iterated over more items than expected; "
		     "found at least %zu, expected %zu", got_nitems,
		     exp_nitems);

	exp = exp_items[iter];

	if (got != exp)
		fail("incorrect item at iter %zu "
		     "(got %p, expected %p)", iter, got, exp);
}

static inline void check_loop_tail(size_t got, size_t exp)
{
	if (got < exp)
		fail("sexpr_for_each iterated over fewer items than expected; "
		     "found %zu, expected %zu", got, exp);

	/* the 'got > exp' case should have been caught by check_loop_body */
	VERIFY3U(got, ==, exp);
}

#define DO_CHECK(loop, list, array, len)				\
	do {								\
		struct val *cur, *tmp;					\
		size_t i = 0;						\
									\
		fprintf(stderr, "%-20s: %2zu items: ", #loop, (len));	\
		safe_dump(list);					\
		fprintf(stderr, "...");					\
									\
		loop(cur, tmp, (list)) {				\
			check_loop_body(cur, (array), i, (len));	\
									\
			i++;						\
		}							\
									\
		check_loop_tail(i, (len));				\
									\
		fprintf(stderr, "ok.\n");				\
	} while (0)

static void test_check(struct val *list, size_t nitems, struct val **items)
{
	DO_CHECK(sexpr_for_each, list, items, nitems);

	val_putref(list);
}

static void test_atoms(void)
{
	test_check(VAL_ALLOC_CHAR('a'), 0, NULL);
	test_check(VAL_ALLOC_INT(7), 0, NULL);
	test_check(VAL_ALLOC_BOOL(true), 0, NULL);
	test_check(VAL_ALLOC_NULL(), 0, NULL);
	test_check(VAL_ALLOC_STR_STATIC("abc"), 0, NULL);
	test_check(VAL_ALLOC_SYM_STATIC("abc"), 0, NULL);
}

static void test_empty(void)
{
	test_check(NULL, 0, NULL);
	test_check(val_empty_cons(), 0, NULL);
}

#define CALL_TEST_CHECK(items) \
	test_check(sexpr_array_to_list((items), ARRAY_LEN(items)), \
		   ARRAY_LEN(items), (items))

static void test_various_lists(void)
{
	struct val *one[1] = {
		VAL_ALLOC_INT(1),
	};
	struct val *two[2] = {
		VAL_ALLOC_INT(1),
		VAL_ALLOC_INT(2),
	};
	struct val *three[3] = {
		VAL_ALLOC_INT(1),
		VAL_ALLOC_INT(2),
		VAL_ALLOC_INT(3),
	};
	struct val *three_cons[3] = {
		VAL_ALLOC_INT(1),
		val_empty_cons(),
		VAL_ALLOC_INT(2),
	};

	CALL_TEST_CHECK(one);
	CALL_TEST_CHECK(two);
	CALL_TEST_CHECK(three);
	CALL_TEST_CHECK(three_cons);
}

static void test_cons(void)
{
	struct val *vals[1] = {
		VAL_ALLOC_INT(1),
	};

	test_check(VAL_ALLOC_CONS(vals[0], VAL_ALLOC_INT(2)),
		   ARRAY_LEN(vals), vals);
}

void test(void)
{
	test_atoms();
	test_empty();
	test_various_lists();
	test_cons();
}
