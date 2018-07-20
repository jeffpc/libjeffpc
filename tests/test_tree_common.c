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

#if defined(TEST_TREE_BST)
#define TREE_TREE		bst_tree
#define TREE_NODE		bst_node
#define TREE_COOKIE		bst_cookie
#define TREE_CREATE		bst_create
#define TREE_DESTROY		bst_destroy
#define TREE_ADD		bst_add
#define TREE_FOR_EACH		bst_for_each
#define TREE_NUMNODES		bst_numnodes
#define TREE_DESTROY_NODES	bst_destroy_nodes
#else
#error "Unspecified test type"
#endif

struct node {
	struct TREE_NODE node;
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

static void init(struct TREE_TREE *tree, const char *testname,
		 size_t numnodes)
{
	fprintf(stderr, "Testing %zu (%s)...", numnodes, testname);

	TREE_CREATE(tree, cmp, sizeof(struct node),
		    offsetof(struct node, node));
}

static void insert(struct TREE_TREE *tree, int val)
{
	struct node *node;

	node = malloc(sizeof(struct node));
	ASSERT3P(node, !=, NULL);

	node->v = val;

	TREE_ADD(tree, node);
}

static void __destroy_iter(struct TREE_TREE *tree, size_t expected_numnodes)
{
	struct node *node;
	size_t found_nodes;

	VERIFY3U(TREE_NUMNODES(tree), ==, expected_numnodes);

	found_nodes = 0;
	TREE_FOR_EACH(tree, node)
		found_nodes++;

	VERIFY3U(TREE_NUMNODES(tree), ==, found_nodes);
	VERIFY3U(TREE_NUMNODES(tree), ==, expected_numnodes);
}

static size_t __destroy_destroy(struct TREE_TREE *tree)
{
	struct TREE_COOKIE cookie;
	struct node *node;
	size_t removed_nodes;

	memset(&cookie, 0, sizeof(cookie));
	removed_nodes = 0;

	while ((node = TREE_DESTROY_NODES(tree, &cookie))) {
		memset(node, 0xba, sizeof(struct node));
		removed_nodes++;
	}

	return removed_nodes;
}

static void destroy(struct TREE_TREE *tree, size_t expected_numnodes)
{
	size_t remaining_nodes;

	remaining_nodes = TREE_NUMNODES(tree);

	VERIFY3U(remaining_nodes, ==, expected_numnodes);

	/* test iteration */
	__destroy_iter(tree, expected_numnodes);

	/* test destruction */
	remaining_nodes -= __destroy_destroy(tree);

	VERIFY0(remaining_nodes);
	VERIFY0(TREE_NUMNODES(tree));

	TREE_DESTROY(tree);

	fprintf(stderr, "ok.\n");
}

static void test_nodes(const char *testname, size_t numnodes, ...)
{
	struct TREE_TREE tree;
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
