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

#include <jeffpc/rbtree.h>
#include <jeffpc/error.h>

#include "tree_impl.h"

static inline void set_red(struct tree_node *node, bool red)
{
	if (node)
		node->red = red;
}

static inline bool is_red(struct tree_node *node)
{
	/* NB: NULLs are black by definition */
	return node && node->red;
}

void rb_create(struct rb_tree *tree,
	       int (*cmp)(const void *, const void *),
	       size_t size, size_t off)
{
	ASSERT3U(off + sizeof(struct rb_node), <=, size);

	tree->tree.cmp = cmp;
	tree->tree.root = NULL;
	tree->tree.node_size = size;
	tree->tree.node_off = off;
	tree->tree.num_nodes = 0;
	tree->tree.flavor = TREE_FLAVOR_RED_BLACK;
}

void rb_destroy(struct rb_tree *tree)
{
	memset(tree, 0, sizeof(struct rb_tree));
}

void rb_add(struct rb_tree *tree, void *item)
{
	struct rb_node *orig;

	orig = rb_insert(tree, item);
	if (orig)
		panic("%s(%p, %p) failed: tree already contains desired key",
		      __func__, tree, item);
}

static void rb_rotate(struct tree_tree *tree, struct tree_node *node,
		      enum tree_dir left)
{
	const enum tree_dir right = 1 - left;
	struct tree_node *tmp;

	ASSERT3P(node->children[right], !=, NULL);

	tmp = node->children[right];
	node->children[right] = tmp->children[left];

	if (tmp->children[left])
		tmp->children[left]->parent = node;

	tmp->parent = node->parent;
	if (!node->parent) {
		tree->root = tmp;
	} else {
		node->parent->children[which_dir(node->parent, node)] = tmp;
	}

	tmp->children[left] = node;
	node->parent = tmp;
}

void *rb_insert_here(struct rb_tree *tree, void *newitem,
		     struct rb_cookie *cookie)
{
	struct tree_node *node;
	void *tmp;

	node = obj2node(&tree->tree, newitem);

	/*
	 * First, insert as if it were an unbalanced binary search tree but
	 * with the new node marked as red.
	 */
	tmp = tree_insert_here(&tree->tree, newitem, &cookie->cookie);
	if (tmp)
		return tmp;

	set_red(node, true);

	/*
	 * Second, restore red-black tree invariants.
	 *
	 * Based on algorithm in CLRS.
	 */
	while (node && is_red(node)) {
		struct tree_node *gparent;
		struct tree_node *parent;
		struct tree_node *uncle;

		parent  = node->parent;
		gparent = parent ? parent->parent : NULL;
		uncle   = gparent ?
			gparent->children[1 - which_dir(gparent, parent)] : NULL;

		if (!is_red(parent)) {
			break;
		} else if (is_red(uncle)) {
			set_red(parent, false);
			set_red(uncle, false);
			set_red(gparent, true);
			node = gparent;
		} else {
			if (which_dir(parent, node) != which_dir(gparent, parent)) {
				/*
				 *        G                G
				 *       / \              / \
				 *      /   \            /   \
				 *     P     U  ==>     N     U
				 *      \              /
				 *       \            /
				 *        N          P
				 *
				 * This doesn't fix anything, but it sets up
				 * the tree for the rotation that follows.
				 */
				rb_rotate(&tree->tree, parent,
					  1 - which_dir(parent, node));

				node = parent;
				parent  = node->parent;
				gparent = parent ? parent->parent : NULL;
			}

			if (which_dir(parent, node) == which_dir(gparent, parent)) {
				/*
				 *        G             P
				 *       / \           / \
				 *      /   \         /   \
				 *     P     U  ==>  N     G
				 *    /                     \
				 *   /                       \
				 *  N                         U
				 */
				rb_rotate(&tree->tree, gparent,
					  1 - which_dir(parent, node));
			}

			set_red(parent, false);
			set_red(gparent, true);
		}
	}

	set_red(tree->tree.root, false);

	return NULL; /* no previous node */
}

void rb_remove(struct rb_tree *_tree, void *item)
{
	struct tree_tree *tree = &_tree->tree;
	struct tree_node *parent;
	struct tree_node *child;
	struct tree_node *node;

	node = obj2node(tree, item);

	tree_remove(tree, item, &parent, &child);

	/* we emptied the tree - no reason to bother fixing up */
	if (!tree->root)
		return;

	/* removing a red node cannot violate red-black tree invariants */
	if (is_red(node))
		return;

	node = child;

	while (node != tree->root && !is_red(node)) {
		const enum tree_dir left = which_dir(parent, node);
		const enum tree_dir right = 1 - left;
		struct tree_node *sibling;

		sibling = parent->children[right];

		if (is_red(sibling)) {
			set_red(sibling, false);
			set_red(parent, true);
			rb_rotate(tree, parent, left);
			sibling = parent->children[right];
		}

		if (!is_red(sibling->children[left]) &&
		    !is_red(sibling->children[right])) {
			set_red(sibling, true);
			node = parent;
		} else {
			if (!is_red(sibling->children[right])) {
				set_red(sibling->children[left], false);
				set_red(sibling, true);
				rb_rotate(tree, sibling, right);
				sibling = parent->children[right];
			}

			set_red(sibling, is_red(parent));
			set_red(parent, false);
			set_red(sibling->children[right], false);
			rb_rotate(tree, parent, left);
			node = tree->root;
		}

		parent = node->parent;
	}

	set_red(node, false);
}
