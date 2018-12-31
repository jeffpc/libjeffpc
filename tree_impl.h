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

#ifndef __TREE_IMPL_H
#define __TREE_IMPL_H

#include <jeffpc/tree_private.h>

extern void *tree_insert_here(struct tree_tree *tree, void *newitem,
			      struct tree_cookie *cookie);
extern void tree_remove(struct tree_tree *tree, void *item,
			struct tree_node **parent_r,
			struct tree_node **child_r);

static inline void *node2obj(struct tree_tree *tree, struct tree_node *node)
{
	return (void *)(((uintptr_t) node) - tree->node_off);
}

static inline struct tree_node *obj2node(struct tree_tree *tree, void *obj)
{
	return (struct tree_node *)(((uintptr_t) obj) + tree->node_off);
}

static inline struct tree_node *get_parent(struct tree_node *node)
{
#ifdef JEFFPC_TREE_COMPACT
	return (void *) (node->_parent_and_extra & ~0x3ul);
#else
	return node->_parent;
#endif
}

static inline void set_parent(struct tree_node *node, struct tree_node *parent)
{
#ifdef JEFFPC_TREE_COMPACT
	node->_parent_and_extra = (node->_parent_and_extra & 0x3ul) |
				  (((uintptr_t) parent) & ~0x3ul);
#else
	node->_parent = parent;
#endif
}

static inline unsigned int get_extra(struct tree_node *node)
{
#ifdef JEFFPC_TREE_COMPACT
	return node->_parent_and_extra & 0x3;
#else
	return node->_extra;
#endif
}

static inline void set_extra(struct tree_node *node, unsigned int extra)
{
#ifdef JEFFPC_TREE_COMPACT
	node->_parent_and_extra = (node->_parent_and_extra & ~0x3ul) |
				  (extra & 0x3);
#else
	node->_extra = extra;
#endif
}

static inline enum tree_dir which_dir(struct tree_node *parent,
				      struct tree_node *tgt)
{
	return parent->children[TREE_LEFT] == tgt ? TREE_LEFT : TREE_RIGHT;
}

static inline struct tree_node *firstlast(struct tree_node *cur,
					  enum tree_dir dir)
{
	for (; cur->children[dir]; cur = cur->children[dir])
		;

	return cur;
}

static inline void *firstlast_obj(struct tree_tree *tree, enum tree_dir dir)
{
	if (!tree->root)
		return NULL;

	return node2obj(tree, firstlast(tree->root, dir));
}

static inline void *tree_next_dir(struct tree_tree *tree, void *item, bool fwd)
{
	const enum tree_dir right = fwd ? TREE_RIGHT : TREE_LEFT;
	const enum tree_dir left = 1 - right;
	struct tree_node *node;

	node = obj2node(tree, item);

	if (node->children[right])
		return node2obj(tree, firstlast(node->children[right], left));

	while (get_parent(node)) {
		if (which_dir(get_parent(node), node) == left)
			return node2obj(tree, get_parent(node));
		else if (which_dir(get_parent(node), node) == right)
			node = get_parent(node);
	}

	return NULL;
}

#endif
