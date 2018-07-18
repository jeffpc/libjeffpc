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
		 size_t numnodes, struct node **nodes)
{
	size_t i;

	fprintf(stderr, "Testing %zu node tree (%s)...", numnodes, testname);

	TREE_CREATE(tree, cmp, sizeof(struct node),
		    offsetof(struct node, node));

	/* insert the first set of pointers */
	for (i = 0; i < numnodes; i++)
		TREE_ADD(tree, nodes[i]);

}

static size_t __destroy_destroy(struct TREE_TREE *tree)
{
	struct TREE_COOKIE cookie;
	struct node *node;
	size_t removed_nodes;

	fprintf(stderr, "destroy...");
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

	/* test destruction */
	remaining_nodes -= __destroy_destroy(tree);

	VERIFY0(remaining_nodes);
	VERIFY0(TREE_NUMNODES(tree));

	TREE_DESTROY(tree);

	fprintf(stderr, "ok.\n");
}

static void __verify_nodes(struct tree_node *node, size_t numnodes,
			   struct node **exp, size_t level,
			   size_t leveloff)
{
	size_t idx = (1 << level) - 1 + leveloff;

	if ((idx >= numnodes) || !exp[idx]) {
		VERIFY3P(node, ==, NULL);
		return;
	}

	VERIFY3P(node, ==, &exp[idx]->node.node);

	__verify_nodes(node->children[TREE_LEFT], numnodes, exp,
		       level + 1, leveloff * 2);
	__verify_nodes(node->children[TREE_RIGHT], numnodes, exp,
		       level + 1, leveloff * 2 + 1);
}

static void verify_nodes(struct TREE_TREE *tree, size_t numnodes,
			 struct node **exp)
{
	fprintf(stderr, "verify...");
	__verify_nodes(tree->tree.root, numnodes, exp, 0, 0);
}

static void verify_iter(struct TREE_TREE *tree, size_t expected_numnodes)
{
	struct node *max_node;
	struct node *node;
	size_t found_nodes;
	int max_so_far;

	fprintf(stderr, "iter...");
	VERIFY3U(TREE_NUMNODES(tree), ==, expected_numnodes);

	found_nodes = 0;
	max_so_far = INT_MIN;
	max_node = NULL;
	TREE_FOR_EACH(tree, node) {
		found_nodes++;

		if (max_so_far >= node->v)
			panic("found node with value preceding previous "
			      "(current: %d @ %p, previous: %d @ %p)",
			      node->v, node, max_so_far, max_node);

		max_so_far = node->v;
		max_node = node;
	}

	VERIFY3U(TREE_NUMNODES(tree), ==, found_nodes);
	VERIFY3U(TREE_NUMNODES(tree), ==, expected_numnodes);
}

static void test_nodes(const char *testname,
		       size_t numinsert, struct node **inserts,
		       size_t numcheck, struct node **checks)
{
	struct TREE_TREE tree;

	init(&tree, testname, numinsert, inserts);
	verify_nodes(&tree, numcheck, checks);
	verify_iter(&tree, numinsert);
	destroy(&tree, numinsert);
}

static void test_insert_empty(void)
{
	test_nodes("empty", 0, NULL, 0, NULL);
}

static void test_insert_one(void)
{
	struct node a = { .v = 10 };
	struct node *inserts[1] = { &a };
	struct node *checks[1] = { &a };

	test_nodes("root",
		   ARRAY_LEN(inserts), inserts,
		   ARRAY_LEN(checks), checks);
}

static void test_insert_two(bool asc)
{
	struct node a = { .v = 10 };
	struct node b = { .v = 20 };
	struct node *inserts[2][2] = {
		[true]  = { &a, &b },
		[false] = { &b, &a },
	};
	struct node *checks[2][3] = {
#if defined(TEST_TREE_BST)
		[true]  = { &a, NULL, &b },
		[false] = { &b, &a, NULL },
#endif
	};

	test_nodes("two descending",
		   ARRAY_LEN(inserts[asc]), inserts[asc],
		   ARRAY_LEN(checks[asc]), checks[asc]);
}

static void test_insert_three(int perm)
{
	char name[64];
	struct node a = { .v = 10 };
	struct node b = { .v = 20 };
	struct node c = { .v = 30 };
	struct node *inserts[6][3] = {
		[0] = { &a, &b, &c },
		[1] = { &a, &c, &b },
		[2] = { &b, &a, &c },
		[3] = { &b, &c, &a },
		[4] = { &c, &a, &b },
		[5] = { &c, &b, &a },
	};
	struct node *checks[6][7] = {
#if defined(TEST_TREE_BST)
		[0] = { &a,
			NULL, &b,
			NULL, NULL, NULL, &c },
		[1] = { &a,
			NULL, &c,
			NULL, NULL, &b, NULL },
		[2] = { &b,
			&a, &c },
		[3] = { &b,
			&a, &c },
		[4] = { &c,
			&a, NULL,
			NULL, &b, NULL, NULL },
		[5] = { &c,
			&b, NULL,
			&a, NULL, NULL, NULL },
#endif
	};

	snprintf(name, sizeof(name), "permutation: %d", perm);

	test_nodes(name,
		   ARRAY_LEN(inserts[perm]), inserts[perm],
		   ARRAY_LEN(checks[perm]), checks[perm]);
}

static void test_insert_four(int perm)
{
	char name[64];
	struct node a = { .v = 10 };
	struct node b = { .v = 20 };
	struct node c = { .v = 30 };
	struct node d = { .v = 40 };
	struct node *inserts[24][4] = {
		[0]  = { &a, &b, &c, &d },
		[1]  = { &a, &c, &b, &d },
		[2]  = { &b, &a, &c, &d },
		[3]  = { &b, &c, &a, &d },
		[4]  = { &c, &a, &b, &d },
		[5]  = { &c, &b, &a, &d },

		[6]  = { &a, &b, &d, &c },
		[7]  = { &a, &c, &d, &b },
		[8]  = { &b, &a, &d, &c },
		[9]  = { &b, &c, &d, &a },
		[10] = { &c, &a, &d, &b },
		[11] = { &c, &b, &d, &a },

		[12] = { &a, &d, &b, &c },
		[13] = { &a, &d, &c, &b },
		[14] = { &b, &d, &a, &c },
		[15] = { &b, &d, &c, &a },
		[16] = { &c, &d, &a, &b },
		[17] = { &c, &d, &b, &a },

		[18] = { &d, &a, &b, &c },
		[19] = { &d, &a, &c, &b },
		[20] = { &d, &b, &a, &c },
		[21] = { &d, &b, &c, &a },
		[22] = { &d, &c, &a, &b },
		[23] = { &d, &c, &b, &a },
	};

	/* various balanced orderings */
#define TEST4_BACD	{ &b, \
			  &a, &c, \
			  NULL, NULL, NULL, &d }
#define TEST4_BADC	{ &b, \
			  &a, &d, \
			  NULL, NULL, &c, NULL }
#define TEST4_CADB	{ &c, \
			  &a, &d, \
			  NULL, &b, NULL, NULL }
#define TEST4_CBDA	{ &c, \
			  &b, &d, \
			  &a, NULL, NULL, NULL }

	struct node *checks[24][15] = {
#if defined(TEST_TREE_BST)
		[0]  = { &a,
			 NULL, &b,
			 NULL, NULL, NULL, &c,
			 NULL, NULL, NULL, NULL, NULL, NULL, NULL, &d },
		[1]  = { &a,
			 NULL, &c,
			 NULL, NULL, &b, &d },
		[2]  = TEST4_BACD,
		[3]  = TEST4_BACD,
		[4]  = TEST4_CADB,
		[5]  = TEST4_CBDA,

		[6]  = { &a,
			 NULL, &b,
			 NULL, NULL, NULL, &d,
			 NULL, NULL, NULL, NULL, NULL, NULL, &c, NULL },
		[7]  = { &a,
			 NULL, &c,
			 NULL, NULL, &b, &d },
		[8]  = TEST4_BADC,
		[9]  = TEST4_BACD,
		[10] = TEST4_CADB,
		[11] = TEST4_CBDA,

		[12] = { &a,
			 NULL, &d,
			 NULL, NULL, &b, NULL,
			 NULL, NULL, NULL, NULL, NULL, &c, NULL, NULL },
		[13] = { &a,
			 NULL, &d,
			 NULL, NULL, &c, NULL,
			 NULL, NULL, NULL, NULL, &b, NULL, NULL, NULL },
		[14] = TEST4_BADC,
		[15] = TEST4_BADC,
		[16] = TEST4_CADB,
		[17] = TEST4_CBDA,

		[18] = { &d,
			 &a, NULL,
			 NULL, &b, NULL, NULL,
			 NULL, NULL, NULL, &c, NULL, NULL, NULL, NULL },
		[19] = { &d,
			 &a, NULL,
			 NULL, &c, NULL, NULL,
			 NULL, NULL, &b, NULL, NULL, NULL, NULL, NULL },
		[20] = { &d,
			 &b, NULL,
			 &a, &c, NULL, NULL},
		[21] = { &d,
			 &b, NULL,
			 &a, &c, NULL, NULL},
		[22] = { &d,
			 &c, NULL,
			 &a, NULL, NULL, NULL,
			 NULL, &b, NULL, NULL, NULL, NULL, NULL, NULL },
		[23] = { &d,
			 &c, NULL,
			 &b, NULL, NULL, NULL,
			 &a, NULL, NULL, NULL, NULL, NULL, NULL, NULL },
#endif
	};

	snprintf(name, sizeof(name), "permutation: %d", perm);

	test_nodes(name,
		   ARRAY_LEN(inserts[perm]), inserts[perm],
		   ARRAY_LEN(checks[perm]), checks[perm]);
}

static void test_insert(void)
{
	int i;

	test_insert_empty();
	test_insert_one();
	test_insert_two(true);
	test_insert_two(false);
	for (i = 0; i < 6; i++)
		test_insert_three(i);
	for (i = 0; i < 24; i++)
		test_insert_four(i);
}

void test(void)
{
	test_insert();
}
