/*
 * Copyright (c) 2017-2018 Josef 'Jeff' Sipek <jeffpc@josefsipek.net>
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

#include <string.h>

#include <jeffpc/bst.h>
#include <jeffpc/error.h>

#include "tree_impl.h"

void bst_create(struct bst_tree *tree,
		int (*cmp)(const void *, const void *),
		size_t size, size_t off)
{
	ASSERT3U(off + sizeof(struct bst_node), <=, size);

	tree->tree.cmp = cmp;
	tree->tree.root = NULL;
	tree->tree.node_size = size;
	tree->tree.node_off = off;
	tree->tree.num_nodes = 0;
	tree->tree.flavor = TREE_FLAVOR_UNBALANCED;
}

void bst_destroy(struct bst_tree *tree)
{
	memset(tree, 0, sizeof(struct bst_tree));
}

void bst_add(struct bst_tree *tree, void *item)
{
	struct bst_node *orig;

	orig = bst_insert(tree, item);
	if (orig)
		panic("%s(%p, %p) failed: tree already contains desired key",
		      __func__, tree, item);
}

void *bst_insert_here(struct bst_tree *tree, void *newitem,
		      struct bst_cookie *cookie)
{
	return tree_insert_here(&tree->tree, newitem, &cookie->cookie);
}

static inline void __bst_swap_nodes(struct tree_tree *tree,
				    struct tree_node *x,
				    struct tree_node *y,
				    const enum tree_dir left)
{
	const enum tree_dir right = 1 - left;
	const bool root = tree->root == x;
	enum tree_dir dir_to_orig_x;

	ASSERT3P(x, !=, y);
	ASSERT3P(y->children[left], ==, NULL);

	if (!root)
		dir_to_orig_x = which_dir(x->parent, x);

	if (x->children[right] == y) {
		/*
		 *  A                 A
		 *   \                 \
		 *    \                 \
		 *     x                 y
		 *    / \               / \
		 *   /   \       =>    /   \
		 *  B     y           B     x
		 *         \                 \
		 *          \                 \
		 *           E                 E
		 */
		struct tree_node *A, *B, *E;

		A = x->parent;
		B = x->children[left];
		E = y->children[right];

		x->parent = y;
		y->parent = A;
		B->parent = y;
		if (E)
			E->parent = x;

		x->children[left] = NULL;
		x->children[right] = E;
		y->children[left] = B;
		y->children[right] = x;
		if (!root)
			A->children[dir_to_orig_x] = y;
		else
			tree->root = y;
	} else {
		/*
		 *    A                 A
		 *     \                 \
		 *      \                 \
		 *       x                 y
		 *      / \               / \
		 *     /   \       =>    /   \
		 *    B     C           B     C
		 *         /                 /
		 *        .                 .
		 *       .                 .
		 *      /                 /
		 *     D                 D
		 *    /                 /
		 *   /                 /
		 *  y                 x
		 *   \                 \
		 *    \                 \
		 *     E                 E
		 */
		struct tree_node *A, *B, *C, *D, *E;

		A = x->parent;
		B = x->children[left];
		C = x->children[right];
		D = y->parent;
		E = y->children[right];

		x->parent = D;
		y->parent = A;
		B->parent = y;
		C->parent = y;
		if (E)
			E->parent = x;

		x->children[left] = NULL;
		x->children[right] = E;
		y->children[left] = B;
		y->children[right] = C;
		if (!root)
			A->children[dir_to_orig_x] = y;
		else
			tree->root = y;
		D->children[left] = x;
	}
}

static inline void __bst_remove_node(struct tree_tree *tree,
				     struct tree_node *parent,
				     struct tree_node *tgt,
				     struct tree_node *child)
{
	if (parent)
		parent->children[which_dir(parent, tgt)] = child;
	else
		tree->root = child;

	if (child)
		child->parent = parent;
}

void bst_remove(struct bst_tree *_tree, void *item)
{
	struct tree_tree *tree = &_tree->tree;
	struct tree_node *node;

	ASSERT3P(tree->root, !=, NULL);
	ASSERT3P(item, !=, NULL);

	node = obj2node(tree, item);

	if (node->children[TREE_LEFT] && node->children[TREE_RIGHT])
		/*
		 * Two children: exchange it with a leaf-ish node greater
		 * than this node.
		 */
		__bst_swap_nodes(tree, node,
				 firstlast(node->children[TREE_RIGHT], TREE_LEFT),
				 TREE_LEFT);

	/* now, we have zero or one child */
	ASSERT(!node->children[TREE_LEFT] || !node->children[TREE_RIGHT]);

	if (node->children[TREE_LEFT])
		__bst_remove_node(tree, node->parent, node,
				  node->children[TREE_LEFT]);
	else if (node->children[TREE_RIGHT])
		__bst_remove_node(tree, node->parent, node,
				  node->children[TREE_RIGHT]);
	else
		__bst_remove_node(tree, node->parent, node, NULL);

	node->parent = NULL;
	node->children[TREE_LEFT] = NULL;
	node->children[TREE_RIGHT] = NULL;

	tree->num_nodes--;
}
