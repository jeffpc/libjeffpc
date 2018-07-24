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

void bst_remove(struct bst_tree *tree, void *item)
{
	tree_remove(&tree->tree, item);
}
