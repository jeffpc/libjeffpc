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

#define DESTROY_NODES_DONE	((struct tree_node *)(uintptr_t)~0ul)

static inline void destroy_nodes_save_parent(struct tree_tree *tree,
					     struct tree_node *node,
					     struct tree_cookie *cookie)
{
	if (node != tree->root) {
		cookie->node = node->parent;
		cookie->dir = 1 - which_dir(node->parent, node);
	} else {
		/* indicate end of iteration */
		cookie->node = DESTROY_NODES_DONE;
	}
}

static struct tree_node *destroy_nodes_step(struct tree_tree *tree,
					    struct tree_node *node,
					    struct tree_cookie *cookie)
{
	ASSERT3P(node, !=, NULL);

	for (;;) {
		struct tree_node *tmp;

		tmp = firstlast(node, TREE_LEFT);

		if (!tmp->children[TREE_RIGHT]) {
			destroy_nodes_save_parent(tree, tmp, cookie);
			return tmp;
		}

		node = tmp->children[TREE_RIGHT];
	}
}

/*
 * This function does a post-order traversal of the tree, returning each
 * node and using the cookie as a cursor.  Once a node is returned, it's
 * memory is assumed to be free.
 *
 * The cookie's ->node is used as pointer to the next item to process (not
 * necessarily return!) and the ->dir member is used to indicate which child
 * of the next node to look at.
 *
 *   node |  dir  | meaning
 *  ------+-------+------------------------------------------
 *   NULL |   *   | first invocation, return first node
 *   DONE |   *   | all nodes destroyed already, return NULL
 *    *   | RIGHT | process node's right subtree next
 *    *   | LEFT  | process node itself next
 *
 * Note that since we traverse the tree left to right, we go as far left
 * every time we descend to the right.  That is why there is no explicit
 * go-left state.
 */
void *tree_destroy_nodes(struct tree_tree *tree, struct tree_cookie *cookie)
{
	struct tree_node *node;

	if (!tree->root)
		return NULL;

	if (cookie->node == DESTROY_NODES_DONE) {
		tree->root = NULL;
		tree->num_nodes = 0;
		return NULL;
	}

	if (!cookie->node)
		return destroy_nodes_step(tree, tree->root, cookie);

	/* descend down right subtree */
	if (cookie->dir == TREE_RIGHT) {
		node = cookie->node->children[TREE_RIGHT];

		if (node)
			return destroy_nodes_step(tree, node, cookie);
	}

	/* process current node */
	node = cookie->node;

	destroy_nodes_save_parent(tree, cookie->node, cookie);

	return node;
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
