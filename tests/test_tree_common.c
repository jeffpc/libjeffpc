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
#include <jeffpc/rbtree.h>

#include "test.c"

#if defined(TEST_TREE_BST)
#define TREE_TREE		bst_tree
#define TREE_NODE		bst_node
#define TREE_COOKIE		bst_cookie
#define TREE_CREATE		bst_create
#define TREE_DESTROY		bst_destroy
#define TREE_ADD		bst_add
#define TREE_REMOVE		bst_remove
#define TREE_FOR_EACH		bst_for_each
#define TREE_NUMNODES		bst_numnodes
#define TREE_DESTROY_NODES	bst_destroy_nodes
#elif defined(TEST_TREE_RB)
#define TREE_TREE		rb_tree
#define TREE_NODE		rb_node
#define TREE_COOKIE		rb_cookie
#define TREE_CREATE		rb_create
#define TREE_DESTROY		rb_destroy
#define TREE_ADD		rb_add
#define TREE_REMOVE		rb_remove
#define TREE_FOR_EACH		rb_for_each
#define TREE_NUMNODES		rb_numnodes
#define TREE_DESTROY_NODES	rb_destroy_nodes
#else
#error "Unspecified test type"
#endif

struct node {
	struct TREE_NODE node;
	const char *name;
	int v;
};

/* various balanced orderings */
#define TEST3_BAC	{ &b, \
			  &a, &c }
#define TEST3_BAD	{ &b, \
			  &a, &d }
#define TEST3_CAD	{ &c, \
			  &a, &d }
#define TEST3_CBD	{ &c, \
			  &b, &d }

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

static struct node a = { .name = "a", .v = 10 };
static struct node b = { .name = "b", .v = 20 };
static struct node c = { .name = "c", .v = 30 };
static struct node d = { .name = "d", .v = 40 };

struct test2 {
	const char *name;
	struct node *pre[3];
	struct subtest {
		struct node *remove;
		struct node *post[1];
	} sub[2];
};

struct test3 {
	struct node *pre[7];
	struct {
		struct node *remove;
		struct node *nodes[3];
	} sub[3];
};

struct test4 {
	struct node *pre[15];
	struct {
		struct node *remove;
		struct node *nodes[7];
	} sub[4];
};

static struct test2 checks2[2];
static struct test3 checks3[6];
static struct test4 checks4[24];

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

static void init(struct TREE_TREE *tree, const char *testname, const char *nodename,
		 size_t numnodes, struct node **nodes, bool begin)
{
	size_t i;

	if (begin)
		fprintf(stderr, "%zu nodes: %s - %s...", numnodes, testname,
			nodename);

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

	while ((node = TREE_DESTROY_NODES(tree, &cookie)))
		removed_nodes++;

	return removed_nodes;
}

static void destroy(struct TREE_TREE *tree, size_t expected_numnodes,
		    bool done)
{
	size_t remaining_nodes;

	remaining_nodes = TREE_NUMNODES(tree);

	VERIFY3U(remaining_nodes, ==, expected_numnodes);

	/* test destruction */
	remaining_nodes -= __destroy_destroy(tree);

	VERIFY0(remaining_nodes);
	VERIFY0(TREE_NUMNODES(tree));

	TREE_DESTROY(tree);

	if (done)
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

	if (node != &exp[idx]->node.node) {
		panic("node (%p) != expected (%p) at index %zu",
		      node, &exp[idx]->node.node, idx);
	}

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

static void test_nodes(const char *testname, struct node *remove,
		       size_t numinsert, struct node **inserts,
		       size_t npre, struct node **pre,
		       size_t npost, struct node **post)
{
	struct TREE_TREE tree;

	init(&tree, testname, remove ? remove->name : "(none)", numinsert, inserts, true);
#if 0
	fprintf(stderr, "   node dump:\n");
	for (size_t i = 0; i < numinsert; i++)
		fprintf(stderr, "       [%zu] = %p\n", i, inserts[i]);
#endif
	verify_nodes(&tree, npre, pre);
	verify_iter(&tree, numinsert);
	destroy(&tree, numinsert, !remove);

	if (!remove)
		return;

	init(&tree, NULL, NULL, numinsert, inserts, false);
	verify_nodes(&tree, npre, pre);
	verify_iter(&tree, numinsert);
	fprintf(stderr, "remove...");
	TREE_REMOVE(&tree, remove);
	verify_nodes(&tree, npost, post);
	verify_iter(&tree, numinsert - 1);
	destroy(&tree, numinsert - 1, true);
}

static void test_empty(void)
{
	test_nodes("empty", NULL,
		   0, NULL, 0, NULL, 0, NULL);
}

static void test_one(void)
{
	struct node *inserts[1] = { &a };
	struct node *checks[1] = { &a };

	test_nodes("root only", &a,
		   ARRAY_LEN(inserts), inserts,
		   ARRAY_LEN(checks), checks,
		   0, NULL);
}

static void test_two(bool asc)
{
	size_t i;
	struct node *inserts[2][2] = {
		[true]  = { &a, &b },
		[false] = { &b, &a },
	};

	for (i = 0; i < 2; i++)
		test_nodes(checks2[asc].name,
			   checks2[asc].sub[i].remove,
			   ARRAY_LEN(inserts[asc]), inserts[asc],
			   ARRAY_LEN(checks2[asc].pre), checks2[asc].pre,
			   ARRAY_LEN(checks2[asc].sub[i].post), checks2[asc].sub[i].post);
}

static void test_three(int perm)
{
	size_t i;
	struct node *inserts[6][3] = {
		[0] = { &a, &b, &c },
		[1] = { &a, &c, &b },
		[2] = { &b, &a, &c },
		[3] = { &b, &c, &a },
		[4] = { &c, &a, &b },
		[5] = { &c, &b, &a },
	};

	for (i = 0; i < 3 ; i++) {
		char name[64];

		snprintf(name, sizeof(name), "permutation: %d", perm);

		test_nodes(name, checks3[perm].sub[i].remove,
			   ARRAY_LEN(inserts[perm]), inserts[perm],
			   ARRAY_LEN(checks3[perm].pre), checks3[perm].pre,
			   ARRAY_LEN(checks3[perm].sub[i].nodes), checks3[perm].sub[i].nodes);
	}
}

static void test_four(int perm)
{
	size_t i;
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

	for (i = 0; i < 4 ; i++) {
		char name[64];

		snprintf(name, sizeof(name), "permutation: %d", perm);

		test_nodes(name, checks4[perm].sub[i].remove,
			   ARRAY_LEN(inserts[perm]), inserts[perm],
			   ARRAY_LEN(checks4[perm].pre), checks4[perm].pre,
			   ARRAY_LEN(checks4[perm].sub[i].nodes), checks4[perm].sub[i].nodes);
	}
}

static void test_insert(void)
{
	int i;

	test_empty();
	test_one();
	test_two(true);
	test_two(false);
	for (i = 0; i < 6; i++)
		test_three(i);
	for (i = 0; i < 24; i++)
		test_four(i);
}

void test(void)
{
	test_insert();
}
