/*
 * Copyright (c) 2017 Josef 'Jeff' Sipek <jeffpc@josefsipek.net>
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

#ifndef __JEFFPC_LIST_H
#define __JEFFPC_LIST_H

/*
 * Circular doubly-linked list with a sentenel node.
 */

#include <stddef.h>
#include <stdbool.h>

#include <jeffpc/types.h>

struct list_node {
	struct list_node *next;
	struct list_node *prev;
};

static inline void slist_init(struct list_node *node)
{
	node->next = node;
	node->prev = node;
}

static inline bool slist_is_empty(struct list_node *list)
{
	return (list->next == list);
}

#define slist_insert_head(c, n)		slist_insert_after((c), (n))
static inline void slist_insert_after(struct list_node *cur,
				      struct list_node *new)
{
	new->next = cur->next;
	new->prev = cur;
	cur->next = new;
	new->next->prev = new;
}

#define slist_insert_tail(c, n)		slist_insert_before((c), (n))
static inline void slist_insert_before(struct list_node *cur,
				       struct list_node *new)
{
	new->next = cur;
	new->prev = cur->prev;
	cur->prev = new;
	new->prev->next = new;
}

static inline void slist_remove(struct list_node *node)
{
	node->next->prev = node->prev;
	node->prev->next = node->next;

	slist_init(node);
}

#define slist_head(node)	((node)->next)
#define slist_tail(node)	((node)->prev)

#define slist_entry(node, type, member) \
	container_of((node), type, member)

#define slist_head_entry(list, type, member) \
	container_of(slist_head(list), type, member)

#define slist_tail_entry(list, type, member) \
	container_of(slist_tail(list), type, member)

#define slist_for_each(pos, list) \
	for (pos = slist_head(list); \
	     pos != (list); \
	     pos = pos->next)

#define slist_for_each_safe(pos, safe, list) \
	for (pos = slist_head(list), safe = pos->next; \
	     pos != (list); \
	     pos = safe, safe = safe->next)

#define slist_for_each_entry(pos, list, member) \
	for (pos = slist_entry(slist_head(list), typeof(*pos), member); \
	     &pos->member != (list); \
	     pos = slist_entry(pos->member.next, typeof(*pos), member)) \

#define slist_for_each_entry_safe(pos, safe, list, member) \
	for (pos = slist_entry(slist_head(list), typeof(*pos), member), \
	     safe = slist_entry(pos->member.next, typeof(*pos), member); \
	     &pos->member != (list); \
	     pos = safe, \
	     safe = slist_entry(safe->member.next, typeof(*pos), member))

#endif
