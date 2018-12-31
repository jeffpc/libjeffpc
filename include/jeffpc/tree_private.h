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

#ifndef __JEFFPC_TREE_PRIVATE_H
#define __JEFFPC_TREE_PRIVATE_H

#include <stdbool.h>

#include <jeffpc/int.h>

struct tree_node {
	struct tree_node *children[2];
	struct tree_node *_parent;
	bool _extra;
};

struct tree_tree {
	int (*cmp)(const void *, const void *);
	struct tree_node *root;
	size_t node_size;
	size_t node_off;
	size_t num_nodes;
	enum {
		TREE_FLAVOR_UNBALANCED,
		TREE_FLAVOR_RED_BLACK,
	} flavor;
};

struct tree_cookie {
	struct tree_node *node;
	enum tree_dir {
		TREE_LEFT = 0,
		TREE_RIGHT = 1,
	} dir;
};

extern void *tree_find(struct tree_tree *tree, const void *key,
		       struct tree_cookie *cookie);
extern void *tree_nearest(struct tree_tree *tree,
			  struct tree_cookie *cookie, bool gt);
extern void *tree_first(struct tree_tree *tree);
extern void *tree_last(struct tree_tree *tree);
extern void *tree_next(struct tree_tree *tree, void *item);
extern void *tree_prev(struct tree_tree *tree, void *item);
extern void *tree_destroy_nodes(struct tree_tree *tree,
				struct tree_cookie *cookie);
extern void tree_swap(struct tree_tree *tree1, struct tree_tree *tree2);


static inline bool tree_is_empty(struct tree_tree *tree)
{
	return tree->num_nodes == 0;
}

static inline size_t tree_numnodes(struct tree_tree *tree)
{
	return tree->num_nodes;
}

#endif
