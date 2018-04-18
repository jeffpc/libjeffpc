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
#include <jeffpc/rand.h>
#include <jeffpc/time.h>

#define NITERS		100000

struct node {
	struct bst_node node;
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

static void insert(struct bst_tree *tree, struct node *nodes,
		   uint32_t (*f)(void))
{
	uint64_t start, end;
	size_t i, ok;

	ASSERT3U(bst_numnodes(tree), ==, 0);

	start = gettick();
	for (i = 0, ok = 0; i < NITERS; i++) {
		nodes[i].v = f();

		if (!bst_insert(tree, &nodes[i]))
			ok++;
	}
	end = gettick();

	ASSERT3U(bst_numnodes(tree), ==, ok);

	cmn_err(CE_INFO, "%zu/%zu insertions in %"PRIu64" ns", ok, NITERS,
		end - start);
}

static void find_or_delete(struct bst_tree *tree, struct node *nodes,
			   uint32_t (*f)(void), bool find_only)
{
	uint64_t start, end;
	size_t orig_numnodes;
	size_t i, ok;

	orig_numnodes = bst_numnodes(tree);

	start = gettick();
	for (i = 0, ok = 0; i < NITERS * 10; i++) {
		struct node key = {
			.v = f()
		};
		struct node *node;

		node = bst_find(tree, &key, NULL);
		if (node) {
			if (!find_only)
				bst_remove(tree, node);
			ok++;
		}
	}
	end = gettick();

	if (find_only)
		ASSERT3U(bst_numnodes(tree), ==, orig_numnodes);
	else
		ASSERT3U(bst_numnodes(tree), ==, orig_numnodes - ok);

	cmn_err(CE_INFO, "%zu/%zu %s in %"PRIu64" ns", ok, NITERS,
		find_only ? "finds" : "deletions", end - start);
}

static void test(uint32_t (*f)(void), void (*reset)(void))
{
	struct bst_tree tree;
	struct node *nodes;

	bst_create(&tree, cmp, sizeof(struct node),
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

	bst_destroy(&tree);
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