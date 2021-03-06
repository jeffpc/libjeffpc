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

#ifndef __JEFFPC_BST_H
#define __JEFFPC_BST_H

#include <stdbool.h>

#include <jeffpc/tree_private.h>

struct bst_node {
	struct tree_node node;
};

struct bst_tree {
	struct tree_tree tree;
};

struct bst_cookie {
	struct tree_cookie cookie;
};

#define bst_for_each(tree, pos) \
	for (pos = bst_first(tree); pos; pos = bst_next(tree, pos))

extern void bst_create(struct bst_tree *tree,
		       int (*cmp)(const void *, const void *),
		       size_t size, size_t off);
extern void bst_destroy(struct bst_tree *tree);

extern void bst_add(struct bst_tree *tree, void *item);
extern void *bst_insert_here(struct bst_tree *tree, void *item,
			     struct bst_cookie *cookie);
#define bst_insert(tree, item)	bst_insert_here((tree), (item), NULL)
extern void bst_remove(struct bst_tree *tree, void *item);

static inline bool bst_is_empty(struct bst_tree *tree)
{
	return tree_is_empty(&tree->tree);
}

static inline size_t bst_numnodes(struct bst_tree *tree)
{
	return tree_numnodes(&tree->tree);
}

/*
 * Search, iteration, and swapping are completely generic
 */
static inline void *bst_find(struct bst_tree *tree, const void *key,
			     struct bst_cookie *cookie)
{
	return tree_find(&tree->tree, key, &cookie->cookie);
}

static inline void *bst_nearest_lt(struct bst_tree *tree,
				   struct bst_cookie *cookie)
{
	return tree_nearest(&tree->tree, &cookie->cookie, false);
}

static inline void *bst_nearest_gt(struct bst_tree *tree,
				   struct bst_cookie *cookie)
{
	return tree_nearest(&tree->tree, &cookie->cookie, true);
}

static inline void *bst_first(struct bst_tree *tree)
{
	return tree_first(&tree->tree);
}

static inline void *bst_last(struct bst_tree *tree)
{
	return tree_last(&tree->tree);
}

static inline void *bst_next(struct bst_tree *tree, void *item)
{
	return tree_next(&tree->tree, item);
}

static inline void *bst_prev(struct bst_tree *tree, void *item)
{
	return tree_prev(&tree->tree, item);
}

static inline void *bst_destroy_nodes(struct bst_tree *tree,
				      struct bst_cookie *cookie)
{
	return tree_destroy_nodes(&tree->tree, &cookie->cookie);
}

static inline void bst_swap(struct bst_tree *tree1, struct bst_tree *tree2)
{
	tree_swap(&tree1->tree, &tree2->tree);
}

#endif
