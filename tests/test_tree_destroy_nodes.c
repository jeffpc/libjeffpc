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

#include <jeffpc/types.h>
#include <jeffpc/bst.h>

#include "test.c"

struct node {
	struct bst_node node;
	int v;
};

static int cmp(const void *va, const void *vb)
{
	const struct node *a = va;
	const struct node *b = vb;

	if (a->v < b->v)
		return -1;
	if (a->v > b->v)
		return 1;
	return 0;
}

static void init(struct bst_tree *tree, const char *testname,
		 size_t numnodes)
{
	fprintf(stderr, "Testing %zu (%s)...", numnodes, testname);

	bst_create(tree, cmp, sizeof(struct node),
		   offsetof(struct node, node));
}

static void insert(struct bst_tree *tree, int val)
{
	struct node *node;

	node = malloc(sizeof(struct node));
	ASSERT3P(node, !=, NULL);

	node->v = val;

	bst_add(tree, node);
}

static void destroy(struct bst_tree *tree, size_t expected_numnodes)
{
	struct bst_cookie cookie;
	struct bst_node *node;
	size_t remaining_nodes;

	memset(&cookie, 0, sizeof(cookie));

	remaining_nodes = bst_numnodes(tree);

	VERIFY3U(remaining_nodes, ==, expected_numnodes);

	while ((node = bst_destroy_nodes(tree, &cookie))) {
		memset(node, 0xba, sizeof(struct node));
		remaining_nodes--;
	}

	VERIFY0(remaining_nodes);
	VERIFY0(bst_numnodes(tree));

	bst_destroy(tree);

	fprintf(stderr, "ok.\n");
}

static void test_nodes(const char *testname, size_t numnodes, ...)
{
	struct bst_tree tree;
	va_list ap;
	size_t i;

	init(&tree, testname, numnodes);
	va_start(ap, numnodes);
	for (i = 0; i < numnodes; i++)
		insert(&tree, va_arg(ap, int));
	va_end(ap);
	destroy(&tree, numnodes);
}

void test(void)
{
	test_nodes("empty",		0);
	test_nodes("root",		1, 5);
	test_nodes("left-lean",		2, 2, 1);
	test_nodes("right-lean",	2, 1, 2);
	test_nodes("balanced",		3, 2, 1, 3);
	test_nodes("L-L",		3, 3, 2, 1);
	test_nodes("L-R",		3, 3, 1, 2);
	test_nodes("R-R",		3, 1, 2, 3);
	test_nodes("R-L",		3, 1, 3, 2);
	test_nodes("L-R-L-R",		5, 5, 1, 4, 2, 3);
	test_nodes("R-L-R-L",		5, 1, 5, 2, 4, 3);

	test_nodes("complex A", 6,
		   /* row 1 */ 5,
		   /* row 2 */ 3, 6,
		   /* row 3 */ 1, 4,
		   /* row 4 */ 2);
	test_nodes("complex B", 6,
		   /* row 1 */ 5,
		   /* row 2 */ 3, 6,
		   /* row 3 */ 2, 4,
		   /* row 4 */ 1);
	test_nodes("complex C", 11,
		   /* row 1 */ 4,
		   /* row 2 */ 2, 8,
		   /* row 3 */ 1, 3, 6, 10,
		   /* row 4 */ 5, 7, 9, 11);
	test_nodes("complex D", 10,
		   /* row 1 */ 4,
		   /* row 2 */ 2, 8,
		   /* row 3 */ 1, 3, 6, 9,
		   /* row 4 */ 5, 7, 10);
}
