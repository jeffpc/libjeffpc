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

void *tree_find(struct tree_tree *tree, const void *key,
		struct tree_cookie *cookie)
{
	struct tree_node *cur;
	struct tree_cookie where;

	memset(&where, 0, sizeof(where));

	cur = tree->root;

	while (cur) {
		int cmp;

		cmp = tree->cmp(key, node2obj(tree, cur));
		if (cmp == 0)
			return node2obj(tree, cur);

		where.node = cur;
		where.dir = (cmp < 0) ? TREE_LEFT : TREE_RIGHT;

		cur = where.node->children[where.dir];
	}

	if (cookie)
		*cookie = where;

	return NULL;
}

void *tree_insert_here(struct tree_tree *tree, void *newitem,
		       struct tree_cookie *cookie)
{
	struct tree_cookie local_cookie;
	struct tree_node *node;

	if (!cookie) {
		void *tmp;

		tmp = tree_find(tree, newitem, &local_cookie);
		if (tmp)
			return tmp;

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

void *tree_first(struct tree_tree *tree)
{
	return firstlast_obj(tree, TREE_LEFT);
}

void *tree_last(struct tree_tree *tree)
{
	return firstlast_obj(tree, TREE_RIGHT);
}

void *tree_next(struct tree_tree *tree, void *item)
{
	return tree_next_dir(tree, item, true);
}

void *tree_prev(struct tree_tree *tree, void *item)
{
	return tree_next_dir(tree, item, false);
}

void tree_swap(struct tree_tree *tree1, struct tree_tree *tree2)
{
	struct tree_node *tmp_root;
	size_t tmp_num_nodes;

	VERIFY3P(tree1->cmp, ==, tree2->cmp);
	VERIFY3U(tree1->node_size, ==, tree2->node_size);
	VERIFY3U(tree1->node_off, ==, tree2->node_off);
	VERIFY3U(tree1->flavor, ==, tree2->flavor);

	tmp_root = tree1->root;
	tmp_num_nodes = tree1->num_nodes;

	tree1->root = tree2->root;
	tree1->num_nodes = tree2->num_nodes;

	tree2->root = tmp_root;
	tree2->num_nodes = tmp_num_nodes;
}
