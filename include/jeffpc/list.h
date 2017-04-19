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
 *
 * This file implements two different flavors of the linked list.  The
 * simpler of the two relies only on struct list_node.  It is up to the user
 * to know how to convert a struct list_node pointer to a meaningful data
 * node.  All the functions and macros related to this flavor are of the
 * form slist_*.
 *
 * The second, more complicated one, uses struct list as the "root" of the
 * list.  It keeps track of the struct list_node offset, and lets the
 * consumer not worry about the conversion.  It is implemented in terms of
 * the first flavor.  All the functions and macros related to this flavor
 * are of the form list_*.
 */

#include <stddef.h>
#include <stdbool.h>

#include <jeffpc/types.h>

struct list_node {
	struct list_node *next;
	struct list_node *prev;
};

struct list {
	size_t offset;
	size_t size;
	struct list_node head;
};

/*
 * slist
 */

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

static inline void slist_move_tail(struct list_node *dst,
				   struct list_node *src)
{
	if (slist_is_empty(src))
		return;

	dst->prev->next = src->next;
	src->next->prev = dst->prev;
	src->prev->next = dst;
	dst->prev = src->prev;

	slist_init(src);
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

/*
 * list
 */

extern void list_create(struct list *list, size_t size, size_t offset);
extern void list_destroy(struct list *list);
extern void list_move_tail(struct list *dst, struct list *src);

/* hepler function - not for consumer use */
static inline struct list_node *_list_obj2node(struct list *list, void *obj)
{
	uintptr_t addr = (uintptr_t) obj;

	return (struct list_node *) (addr + list->offset);
}

/* hepler function - not for consumer use */
static inline void *_list_node2obj(struct list *list, struct list_node *node)
{
	uintptr_t addr = (uintptr_t) node;

	return (void *) (addr - list->offset);
}

static inline bool list_is_empty(struct list *list)
{
	return slist_is_empty(&list->head);
}

static inline void list_insert_after(struct list *list, void *ref, void *new)
{
	slist_insert_after(_list_obj2node(list, ref),
			   _list_obj2node(list, new));
}

static inline void list_insert_before(struct list *list, void *ref, void *new)
{
	slist_insert_before(_list_obj2node(list, ref),
			    _list_obj2node(list, new));
}

static inline void list_insert_head(struct list *list, void *new)
{
	slist_insert_after(&list->head, _list_obj2node(list, new));
}

static inline void list_insert_tail(struct list *list, void *new)
{
	slist_insert_before(&list->head, _list_obj2node(list, new));
}

/* TODO: is this too big to be a static inline? */
static inline void *list_next(struct list *list, void *ref)
{
	struct list_node *node;

	node = (ref == NULL) ? &list->head : _list_obj2node(list, ref);

	if (node->next == &list->head)
		return NULL;

	return _list_node2obj(list, node->next);
}

/* TODO: is this too big to be a static inline? */
static inline void *list_prev(struct list *list, void *ref)
{
	struct list_node *node;

	node = (ref == NULL) ? &list->head : _list_obj2node(list, ref);

	if (node->prev == &list->head)
		return NULL;

	return _list_node2obj(list, node->prev);
}

/* TODO: is this too big to be a static inline? */
static inline void *list_head(struct list *list)
{
	return list_next(list, NULL);
}

/* TODO: is this too big to be a static inline? */
static inline void *list_tail(struct list *list)
{
	return list_prev(list, NULL);
}

static inline void list_remove(struct list *list, void *item)
{
	slist_remove(_list_obj2node(list, item));
}

/* TODO: is this too big to be a static inline? */
static inline void *list_remove_head(struct list *list)
{
	struct list_node *node;

	if (slist_is_empty(&list->head))
		return NULL;

	node = slist_head(&list->head);

	slist_remove(node);

	return _list_node2obj(list, node);
}

/* TODO: is this too big to be a static inline? */
static inline void *list_remove_tail(struct list *list)
{
	struct list_node *node;

	if (slist_is_empty(&list->head))
		return NULL;

	node = slist_tail(&list->head);

	slist_remove(node);

	return _list_node2obj(list, node);
}

#define list_for_each(node, list) \
	for (node = list_head(list); \
	     node != NULL; \
	     node = list_next(list, node))

#define list_for_each_safe(node, next, list) \
	for (node = list_head(list), next = list_next(list, node); \
	     node != NULL; \
	     node = next, next = list_next(list, next))

#define list_for_each_reverse(node, list) \
	for (node = list_tail(list); \
	     node != NULL; \
	     node = list_prev(list, node))

#define list_for_each_safe_reverse(node, next, list) \
	for (node = list_tail(list), next = list_prev(list, node); \
	     node != NULL; \
	     node = next, next = list_prev(list, next))

#endif
