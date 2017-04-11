/*
 * Copyright (c) 2014-2017 Josef 'Jeff' Sipek <jeffpc@josefsipek.net>
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

#ifndef __JEFFPC_VAL_H
#define __JEFFPC_VAL_H

#include <stdint.h>
#include <stdbool.h>

#include <jeffpc/str.h>
#include <jeffpc/refcnt.h>
#include <jeffpc/error.h>

enum val_type {
	VT_INT = 0,	/* 64-bit uint */
	VT_STR,		/* a struct str string */
	VT_SYM,		/* symbol */
	VT_BOOL,	/* boolean */
	VT_CONS,	/* cons cell */
	VT_CHAR,	/* a single (unicode) character */
};

struct val {
	enum val_type type;
	refcnt_t refcnt;
	union {
		const uint64_t i;
		const bool b;
		struct str * const str;
		const struct {
			struct val *head;
			struct val *tail;
		} cons;

		/*
		 * We want to keep the normal members const to catch
		 * attempts to modify them, but at the same time we need to
		 * initialize them after allocation.  Instead of venturing
		 * into undefined behavior territory full of ugly casts, we
		 * just duplicate the above members without the const and
		 * with much uglier name.
		 *
		 * Do not use the following members unless you are the val
		 * allocation function!
		 */
		uint64_t _set_i;
		bool _set_b;
		struct str *_set_str;
		struct {
			struct val *head;
			struct val *tail;
		} _set_cons;
	};
};

extern struct val *val_alloc_bool(bool v);
extern struct val *val_alloc_char(uint64_t v);
extern struct val *val_alloc_int(uint64_t v);
extern struct val *val_alloc_str(struct str *v);
extern struct val *val_alloc_sym(struct str *v);
extern struct val *val_alloc_cons(struct val *head, struct val *tail);
extern void val_free(struct val *v);
extern void val_dump(struct val *v, int indent);

REFCNT_INLINE_FXNS(struct val, val, refcnt, val_free, NULL)

#define VAL_ALLOC_SYM(v)			\
	({					\
		struct val *_x;			\
		_x = val_alloc_sym(v);		\
		ASSERT(_x);			\
		_x;				\
	})

#define VAL_ALLOC_SYM_CSTR(v)			\
	VAL_ALLOC_SYM(STR_DUP(v))

#define VAL_ALLOC_STR(v)			\
	({					\
		struct val *_x;			\
		_x = val_alloc_str(v);		\
		ASSERT(_x);			\
		_x;				\
	})

#define VAL_ALLOC_CSTR(v)			\
	VAL_ALLOC_STR(STR_ALLOC(v))

#define VAL_ALLOC_CHAR(v)			\
	({					\
		struct val *_x;			\
		_x = val_alloc_char(v);		\
		ASSERT(_x);			\
		_x;				\
	})

#define VAL_ALLOC_INT(v)			\
	({					\
		struct val *_x;			\
		_x = val_alloc_int(v);		\
		ASSERT(_x);			\
		_x;				\
	})

#define VAL_ALLOC_BOOL(v)			\
	({					\
		struct val *_x;			\
		_x = val_alloc_bool(v);		\
		ASSERT(_x);			\
		_x;				\
	})

#define VAL_ALLOC_CONS(head, tail)		\
	({					\
		struct val *_x;			\
		_x = val_alloc_cons((head), (tail));\
		ASSERT(_x);			\
		_x;				\
	})

#define VAL_DUP_CSTR(v)			VAL_ALLOC_STR(STR_DUP(v))

#endif
