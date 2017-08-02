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
#include <jeffpc/list.h>

#include "test.c"

struct node {
	struct list_node node;
	uint64_t i;
};

static struct list_node slist;
static struct list list;

#define START_MSG(s)			\
	fprintf(stderr, "%s: %s...", __func__, (s))
#define OK_MSG()			\
	fprintf(stderr, "ok.\n")

#define check(expr, failmsg)		\
	do { \
		START_MSG(#expr); \
		if (expr) \
			fail(failmsg); \
		OK_MSG(); \
	} while (0)

#define check_loop_never(loopexpr, failmsg)	\
	do { \
		START_MSG(#loopexpr); \
		loopexpr { \
			fail(failmsg); \
		} \
		OK_MSG(); \
	} while (0)

static void test_empty(void)
{
	struct list_node *spos, *ssafe;
	struct node *pos, *safe;

	check(!slist_is_empty(&slist),
	      "slist is empty but slist_is_empty returned false");
	check(!list_is_empty(&list),
	      "list is empty but list_is_empty returned false");

	check(slist_head(&slist) != &slist,
	      "slist is empty but slist_head returned non-self");
	check(list_head(&list) != NULL,
	      "list is empty but list_head returned non-NULL");

	check(slist_tail(&slist) != &slist,
	      "slist is empty but slist_tail returned non-self");
	check(list_tail(&list) != NULL,
	      "list is empty but list_tail returned non-NULL");

	check_loop_never(slist_for_each(spos, &slist),
			 "slist is empty, but slist_for_each looped");
	check_loop_never(list_for_each(pos, &list),
			 "list is empty, but list_for_each looped");

	check_loop_never(slist_for_each_safe(spos, ssafe, &slist),
			 "slist is empty, but slist_for_each_safe looped");
	check_loop_never(list_for_each_safe(pos, safe, &list),
			 "list is empty, but list_for_each_safe looped");

	check_loop_never(slist_for_each_entry(pos, &slist, node),
			 "slist is empty, but slist_for_each_entry looped");
	check_loop_never(slist_for_each_entry_safe(pos, safe, &slist, node),
			 "slist is empty, but slist_for_each-entry_safe looped");
}

void test(void)
{
	list_create(&list, sizeof(struct node),
		    offsetof(struct node, node));
	slist_init(&slist);

	test_empty();
}
