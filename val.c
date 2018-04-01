/*
 * Copyright (c) 2014-2018 Josef 'Jeff' Sipek <jeffpc@josefsipek.net>
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
#include <errno.h>
#include <stddef.h>
#include <stdio.h>
#include <stdbool.h>
#include <inttypes.h>

#include <jeffpc/val.h>
#include <jeffpc/jeffpc.h>
#include <jeffpc/error.h>
#include <jeffpc/types.h>
#include <jeffpc/mem.h>

#include "val_impl.h"

#define INIT_STATIC_VAL(t, memb, val)				\
		{						\
			.type = (t),				\
			.static_struct = true,			\
			.memb = (val),				\
		}

static const struct val val_cons_empty = INIT_STATIC_VAL(VT_CONS, i, 0);
static const struct val val_true = INIT_STATIC_VAL(VT_BOOL, b, true);
static const struct val val_false = INIT_STATIC_VAL(VT_BOOL, b, false);
static const struct val val_null = INIT_STATIC_VAL(VT_NULL, i, 0);
static const struct val val_ints[10] = {
	[0] = INIT_STATIC_VAL(VT_INT, i, 0),
	[1] = INIT_STATIC_VAL(VT_INT, i, 1),
	[2] = INIT_STATIC_VAL(VT_INT, i, 2),
	[3] = INIT_STATIC_VAL(VT_INT, i, 3),
	[4] = INIT_STATIC_VAL(VT_INT, i, 4),
	[5] = INIT_STATIC_VAL(VT_INT, i, 5),
	[6] = INIT_STATIC_VAL(VT_INT, i, 6),
	[7] = INIT_STATIC_VAL(VT_INT, i, 7),
	[8] = INIT_STATIC_VAL(VT_INT, i, 8),
	[9] = INIT_STATIC_VAL(VT_INT, i, 9),
};

static struct mem_cache *val_cache;

static void __attribute__((constructor)) init_val_subsys(void)
{
	val_cache = mem_cache_create("val-cache", sizeof(struct val), 0);
	ASSERT(!IS_ERR(val_cache));
}

struct val *__val_alloc(enum val_type type)
{
	struct val *val;

	val = mem_cache_alloc(val_cache);
	if (!val)
		return ERR_PTR(-ENOMEM);

	val->type = type;
	val->static_struct = false;

	refcnt_init(&val->refcnt, 1);

	return val;
}

void val_free(struct val *val)
{
	ASSERT(val);
	ASSERT3U(refcnt_read(&val->refcnt), ==, 0);

	switch (val->type) {
		case VT_NULL:
		case VT_INT:
		case VT_BOOL:
		case VT_CHAR:
			break;
		case VT_BLOB:
			if (!val->static_alloc)
				free(val->_set_blob.ptr);
			break;
		case VT_STR:
		case VT_SYM:
			if (!val->static_alloc)
				free((char *) val->_set_str_ptr);
			break;
		case VT_CONS:
			val_putref(val->cons.head);
			val_putref(val->cons.tail);
			break;
		case VT_ARRAY: {
			size_t i;

			for (i = 0; i < val->array.nelem; i++)
				val_putref(val->_set_array.vals[i]);

			if (!val->static_alloc)
				free(val->_set_array.vals);
			break;
		}
		case VT_NVL:
			__val_free_nvl(val);
			break;
	}

	mem_cache_free(val_cache, val);
}

#define DEF_VAL_SET(fxn, vttype, valelem, ctype, putref)	\
struct val *val_alloc_##fxn(ctype v)				\
{								\
	struct val *val;					\
								\
	val = __val_alloc(vttype);				\
	if (IS_ERR(val)) {					\
		putref(v);					\
		return val;					\
	}							\
								\
	val->_set_##valelem = v;				\
								\
	return val;						\
}

static DEF_VAL_SET(int_heap, VT_INT, i, uint64_t, (void))
DEF_VAL_SET(char, VT_CHAR, i, uint64_t, (void))

struct val *val_alloc_int(uint64_t i)
{
	if (i < ARRAY_LEN(val_ints)) {
		/*
		 * Cast away the const - we define the static as const to get it
		 * into .rodata, but we have to drop the const since everything
		 * expects struct val to be writable (because refcounts modify it).
		 * In this case, we won't get any modifications because we're marked
		 * as static.
		 */
		return (struct val *) &val_ints[i];
	}

	return val_alloc_int_heap(i);
}

struct val *val_alloc_bool(bool b)
{
	const struct val *ret;

	ret = b ? &val_true : &val_false;

	/*
	 * Cast away the const - we define the static as const to get it
	 * into .rodata, but we have to drop the const since everything
	 * expects struct val to be writable (because refcounts modify it).
	 * In this case, we won't get any modifications because we're marked
	 * as static.
	 */
	return (struct val *) ret;
}

struct val *val_alloc_null(void)
{
	/*
	 * Cast away the const - we define the static as const to get it
	 * into .rodata, but we have to drop the const since everything
	 * expects struct val to be writable (because refcounts modify it).
	 * In this case, we won't get any modifications because we're marked
	 * as static.
	 */
	return (struct val *) &val_null;
}

static struct val *__val_alloc_blob(void *ptr, size_t size, bool heap)
{
	struct val *val;

	val = __val_alloc(VT_BLOB);
	if (IS_ERR(val)) {
		if (heap)
			free(ptr);

		return val;
	}

	val->_set_blob.ptr = ptr;
	val->_set_blob.size = size;
	val->static_alloc = !heap;

	return val;
}

struct val *val_alloc_blob(void *ptr, size_t size)
{
	return __val_alloc_blob(ptr, size, true);
}

struct val *val_alloc_blob_dup(const void *ptr, size_t size)
{
	void *tmp;

	tmp = malloc(size);
	if (!tmp)
		return ERR_PTR(-ENOMEM);

	memcpy(tmp, ptr, size);

	return __val_alloc_blob(tmp, size, true);
}

struct val *val_alloc_blob_static(const void *ptr, size_t size)
{
	return __val_alloc_blob((void *) ptr, size, false);
}

struct val *val_empty_cons(void)
{
	/*
	 * Cast away the const - we define the static as const to get it
	 * into .rodata, but we have to drop the const since everything
	 * expects struct val to be writable (because refcounts modify it).
	 * In this case, we won't get any modifications because we're marked
	 * as static.
	 */
	return (struct val *) &val_cons_empty;
}

struct val *val_alloc_cons(struct val *head, struct val *tail)
{
	struct val *val;

	if (!head && !tail)
		return val_empty_cons();

	val = __val_alloc(VT_CONS);
	if (IS_ERR(val)) {
		val_putref(head);
		val_putref(tail);
		return val;
	}

	val->_set_cons.head = head;
	val->_set_cons.tail = tail;

	return val;
}
