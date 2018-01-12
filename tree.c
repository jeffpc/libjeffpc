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

#include <jeffpc/error.h>

#include "tree_impl.h"

void *tree_insert_here(struct tree_tree *tree, void *newitem,
		       struct tree_cookie *cookie)
{
	struct tree_cookie local_cookie;
	struct tree_node *node;

	if (!cookie) {
		struct tree_node *tmp;

		tmp = __tree_find(tree, newitem, &local_cookie);
		if (tmp)
			return node2obj(tree, tmp);

		cookie = &local_cookie;
	}

	ASSERT(cookie->dir == TREE_LEFT || cookie->dir == TREE_RIGHT);

	tree->num_nodes++;

	node = obj2node(tree, newitem);
	node->children[TREE_LEFT] = NULL;
	node->children[TREE_RIGHT] = NULL;
	node->parent = cookie->node;

	if (cookie->node)
		cookie->node->children[cookie->dir] = node;
	else
		tree->root = node;

	return NULL;
}
