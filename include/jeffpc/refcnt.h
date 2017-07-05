/*
 * Copyright (c) 2015-2017 Josef 'Jeff' Sipek <jeffpc@josefsipek.net>
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

/*
 * Reference counting is a very common task.  Often enough, this leads to
 * ad-hoc implementations with inconsistent interfaces with strange quirks.
 * This header implements a generic reference counting API that can be used
 * by any structure.
 *
 * At the heart of it is the refcnt_t type.  It maintains a (32-bit unsigned)
 * reference counter.  When the reference count reaches zero, the structure
 * containing it is freed.
 *
 * Using the refcnt API is simple.  First, embed a refcnt_t member into the
 * structure you wish to reference count (the member name can be anything).
 * In your allocation/initialization function initialize the reference count
 * to one (i.e., call refcnt_init(<refcnt>, 1)), and implement a "free"
 * function that takes a pointer to your structure and returns void.
 * Finally, generate the getref and putref functions using REFCNT_PROTOTYPES
 * and REFCNT_FXNS (or REFCNT_INLINE_FXNS if you prefer static inline
 * functions).
 *
 * For example, say we want to reference count struct foo.  We would end up
 * adding the reference count member:
 *
 * struct foo {
 *	... other members ...
 *	refcnt_t refcnt;
 *	... other members ...
 * };
 *
 * Initialize it to 1 when struct foo is allocated:
 *
 * struct foo *alloc_foo(void)
 * {
 *	struct foo *ret;
 *	ret = malloc(sizeof(struct foo));
 *	... other initialization ...
 *	refcnt_init(&ret->refcnt, 1);
 *	return ret;
 * }
 *
 * Make a free function:
 *
 * void free_foo(struct foo *x)
 * {
 *	... other deinitialization ...
 *	free(x);
 * }
 *
 * And either generate the prototypes for foo_getref and foo_putref in a
 * header and the actual functions in the .c file:
 *
 * REFCNT_PROTOTYPES(struct foo, foo);
 * REFCNT_FXNS(struct foo, foo, refcnt, free_foo, NULL);
 *
 * or generate static inline versions in the header:
 *
 * REFCNT_INLINE_FXNS(struct foo, foo, refcnt, free_foo, NULL);
 *
 * Either way, the macros generate two functions with the same signature.
 * (The macros' first arg specifies the type to be reference counted, the
 * second argument the generated function name prefix, the third the
 * refcnt_t member name, the fourth the name of the free function, and the
 * fifth a is-static predicate function pointer).  For example, the above
 * invocations would produce:
 *
 * struct foo *foo_getref(struct foo *x);
 * void foo_putref(struct foo *x);
 *
 * The getref function takes a pointer to the structure that should have its
 * reference count incremented and increments it.  It returns the same
 * pointer for convenience (e.g., allowing expressions like:
 * b = foo_getref(a)).  Trying to increase the reference of NULL is a no-op.
 * Trying to get a reference on a pointer with a zero reference count
 * asserts since it is an indication of a use-after-free bug.
 *
 * The putref function takes a pointer to the structure that should have its
 * reference count decremented and decrements it.  If after the decrementing
 * the reference count is zero, the free function is called on the pointer.
 * Trying to release a reference of NULL is a no-op.  Trying to release a
 * reference when the reference count is already zero asserts since it is an
 * indication of a use-after-free bug.
 *
 * TODO: document the is-static macro argument
 */

#ifndef __JEFFPC_REFCNT_H
#define __JEFFPC_REFCNT_H

#include <stdbool.h>

#include <jeffpc/atomic.h>
#include <jeffpc/error.h>

typedef struct {
	atomic_t count;
} refcnt_t;

static inline void refcnt_init(refcnt_t *x, uint32_t v)
{
	atomic_set(&x->count, v);
}

static inline uint32_t refcnt_read(refcnt_t *x)
{
	return atomic_read(&x->count);
}

/* INTERNAL FUNCTION - DO NOT USE DIRECTLY */
static inline void __refcnt_inc(refcnt_t *x)
{
	atomic_inc(&x->count);
}

/* INTERNAL FUNCTION - DO NOT USE DIRECTLY */
static inline uint32_t __refcnt_dec(refcnt_t *x)
{
	return atomic_dec(&x->count);
}

/*
 * Declare <name>_getref and <name>_putref extern functions.
 */
#define REFCNT_PROTOTYPES(type, name)					\
extern type *name##_getref(type *);					\
extern void name##_putref(type *);

#define __REFCNT_FXNS(vol, type, name, member, freefxn, isstaticfxn)	\
vol type *name##_getref(type *x)					\
{									\
	bool (*isstatic)(type *) = isstaticfxn;				\
									\
	if (!x)								\
		return NULL;						\
									\
	if (!isstatic || !isstatic(x)) {				\
		ASSERT3U(refcnt_read(&x->member), >=, 1);		\
									\
		__refcnt_inc(&x->member);				\
	}								\
									\
	return x;							\
}									\
vol void name##_putref(type *x)						\
{									\
	bool (*isstatic)(type *) = isstaticfxn;				\
									\
	if (!x || (isstatic && isstatic(x)))				\
		return;							\
									\
	ASSERT3S(refcnt_read(&x->member), >=, 1);			\
									\
	if (!__refcnt_dec(&x->member))					\
		freefxn(x);						\
}

/*
 * Define <name>_getref and <name>_putref functions.
 */
#define REFCNT_FXNS(type, name, member, freefxn, isstaticfxn)		\
	__REFCNT_FXNS(, type, name, member, freefxn, isstaticfxn)

/* Define <name>_getref and <name>_putref static inline functions. */
#define REFCNT_INLINE_FXNS(type, name, member, freefxn, isstaticfxn)	\
	__REFCNT_FXNS(static inline, type, name, member, freefxn, isstaticfxn)

#endif
