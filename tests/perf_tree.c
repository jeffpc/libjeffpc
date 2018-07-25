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

#include <jeffpc/jeffpc.h>
#include <jeffpc/error.h>
#include <jeffpc/bst.h>
#include <jeffpc/rbtree.h>
#include <jeffpc/rand.h>
#include <jeffpc/time.h>

#define NITERS		1000000

#define PERF_TREE_RB

#if defined(PERF_TREE_BST)
/*
 * Use fewer iterations to avoid pathological behavior with sequential
 * insertions taking O(n^2) with a large n.
 */
#undef NITERS
#define NITERS			100000
#define TREE_TREE		bst_tree
#define TREE_NODE		bst_node
#define TREE_COOKIE		bst_cookie
#define TREE_CREATE		bst_create
#define TREE_DESTROY		bst_destroy
#define TREE_FIND		bst_find
#define TREE_INSERT		bst_insert
#define TREE_REMOVE		bst_remove
#define TREE_FOR_EACH		bst_for_each
#define TREE_NUMNODES		bst_numnodes
#define TREE_DESTROY_NODES	bst_destroy_nodes
#elif defined(PERF_TREE_RB)
#define TREE_TREE		rb_tree
#define TREE_NODE		rb_node
#define TREE_COOKIE		rb_cookie
#define TREE_CREATE		rb_create
#define TREE_DESTROY		rb_destroy
#define TREE_FIND		rb_find
#define TREE_INSERT		rb_insert
#define TREE_REMOVE		rb_remove
#define TREE_FOR_EACH		rb_for_each
#define TREE_NUMNODES		rb_numnodes
#define TREE_DESTROY_NODES	rb_destroy_nodes
#else
#error "Unspecified test type"
#endif

struct node {
	struct TREE_NODE node;
	uint32_t v;
};

static const char *test_name;

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

static uint32_t myrand(void)
{
	return rand32() % (NITERS * 10);
}

static void nop_reset(void)
{
}

static uint32_t seq_v;

static uint32_t seq(void)
{
	return ++seq_v;
}

static void seq_reset(void)
{
	seq_v = 0;
}

static void insert(struct TREE_TREE *tree, struct node *nodes,
		   uint32_t (*f)(void))
{
	uint64_t start, end;
	size_t i, ok;

	ASSERT3U(TREE_NUMNODES(tree), ==, 0);

	start = gettick();
	for (i = 0, ok = 0; i < NITERS; i++) {
		nodes[i].v = f();

		if (!TREE_INSERT(tree, &nodes[i]))
			ok++;
	}
	end = gettick();

	ASSERT3U(TREE_NUMNODES(tree), ==, ok);

	cmn_err(CE_INFO, "%zu/%zu insertions in %"PRIu64" ns", ok, NITERS,
		end - start);
}

static void find_or_delete(struct TREE_TREE *tree, struct node *nodes,
			   uint32_t (*f)(void), bool find_only)
{
	uint64_t start, end;
	size_t orig_numnodes;
	size_t i, ok;

	orig_numnodes = TREE_NUMNODES(tree);

	start = gettick();
	for (i = 0, ok = 0; i < NITERS * 10; i++) {
		struct node key = {
			.v = f()
		};
		struct node *node;

		node = TREE_FIND(tree, &key, NULL);
		if (node) {
			if (!find_only)
				TREE_REMOVE(tree, node);
			ok++;
		}
	}
	end = gettick();

	if (find_only)
		ASSERT3U(TREE_NUMNODES(tree), ==, orig_numnodes);
	else
		ASSERT3U(TREE_NUMNODES(tree), ==, orig_numnodes - ok);

	cmn_err(CE_INFO, "%zu/%zu %s in %"PRIu64" ns", ok, NITERS,
		find_only ? "finds" : "deletions", end - start);
}

static void test(uint32_t (*f)(void), void (*reset)(void))
{
	struct TREE_TREE tree;
	struct node *nodes;

	TREE_CREATE(&tree, cmp, sizeof(struct node),
		    offsetof(struct node, node));

	nodes = calloc(NITERS, sizeof(struct node));
	if (!nodes)
		panic("Failed to allocate new nodes (niters = %zu)", NITERS);

	reset();
	insert(&tree, nodes, f);
	reset();
	find_or_delete(&tree, nodes, f, true);
	reset();
	find_or_delete(&tree, nodes, f, false);

	free(nodes);

	TREE_DESTROY(&tree);
}

static void usage(const char *prog)
{
	fprintf(stderr, "Usage: %s <test> ...\n", prog);
	fprintf(stderr, "\n");
	fprintf(stderr, "  <test> is one of the following:\n");
	fprintf(stderr, "      rand - random numbers\n");
	fprintf(stderr, "      seq  - monotonically increasing numbers\n");
	exit(1);
}

static const char *get_test_name(void)
{
	return test_name;
}

int main(int argc, char **argv)
{
	struct jeffpc_ops init_ops = {
		.get_session = get_test_name,
	};
	int i;

	jeffpc_init(&init_ops);

	if (argc < 2)
		usage(argv[0]);

	for (i = 1; i < argc; i++) {
		test_name = argv[i];

		if (!strcmp(argv[i], "rand"))
			test(myrand, nop_reset);
		else if (!strcmp(argv[i], "seq"))
			test(seq, seq_reset);
		else
			usage(argv[0]);
	}

	return 0;
}
