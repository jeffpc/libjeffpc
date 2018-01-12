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

#ifndef __JEFFPC_RBTREE_H
#define __JEFFPC_RBTREE_H

#include <stdbool.h>

#include <jeffpc/tree_private.h>

struct rb_node {
	struct tree_node node;
};

struct rb_tree {
	struct tree_tree tree;
};

struct rb_cookie {
	struct tree_cookie cookie;
};

#define rb_for_each(tree, pos) \
	for (pos = rb_first(tree); pos; pos = rb_next(tree, pos))

extern void rb_create(struct rb_tree *tree,
		      int (*cmp)(const void *, const void *),
		      size_t size, size_t off);
extern void rb_destroy(struct rb_tree *tree);

extern void rb_add(struct rb_tree *tree, void *item);
extern void *rb_insert_here(struct rb_tree *tree, void *item,
			    struct rb_cookie *cookie);
#define rb_insert(tree, item)	rb_insert_here((tree), (item), NULL)
extern void rb_remove(struct rb_tree *tree, void *item);

static inline bool rb_is_empty(struct rb_tree *tree)
{
	return tree_is_empty(&tree->tree);
}

static inline size_t rb_numnodes(struct rb_tree *tree)
{
	return tree_numnodes(&tree->tree);
}

/*
 * Search, iteration, and swapping are completely generic
 */
static inline void *rb_find(struct rb_tree *tree, const void *key,
			    struct rb_cookie *cookie)
{
	return tree_find(&tree->tree, key, &cookie->cookie);
}

static inline void *rb_first(struct rb_tree *tree)
{
	return tree_first(&tree->tree);
}

static inline void *rb_last(struct rb_tree *tree)
{
	return tree_last(&tree->tree);
}

static inline void *rb_next(struct rb_tree *tree, void *item)
{
	return tree_next(&tree->tree, item);
}

static inline void *rb_prev(struct rb_tree *tree, void *item)
{
	return tree_prev(&tree->tree, item);
}

static inline void *rb_destroy_nodes(struct rb_tree *tree,
				     struct rb_cookie *cookie)
{
	return tree_destroy_nodes(&tree->tree, &cookie->cookie);
}

static inline void rb_swap(struct rb_tree *tree1, struct rb_tree *tree2)
{
	tree_swap(&tree1->tree, &tree2->tree);
}

#endif
