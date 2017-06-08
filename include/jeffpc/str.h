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

#ifndef __JEFFPC_STR_H
#define __JEFFPC_STR_H

#include <string.h>
#include <stdbool.h>

#include <jeffpc/refcnt.h>

/* ref-counted string */

#define STR_INLINE_LEN	15

struct str {
	union {
		char *str;
		char inline_str[STR_INLINE_LEN + 1];
	};
	/*
	 * FIXME:
	 * We need to somehow convince the compiler that the union can be a
	 * strange number of bytes long (ideally 19).  Then, the bools below
	 * would take up a byte leaving the refcnt at offset 20 - turning the
	 * whole struct into 24-bytes with zero padding and 18 bytes for the
	 * inline string (instead of the current 15).
	 *
	 * Instead, we get the union of 16 bytes, the bools byte, 3 bytes of
	 * padding, and 4 bytes for the refcnt.
	 *
	 * We cannot simply add the packed attribute onto the union since
	 * that would generate terrible code when trying to access the ->str
	 * pointer.  Adding packed,aligned(8) or something like that
	 * generates padding.  (Actually, aligned(8) makes the union a
	 * multiple of 8 bytes creating tons of padding!)
	 */
	bool static_struct:1;	/* struct str is static */
	bool static_alloc:1;	/* char * is static */
	bool inline_alloc:1;	/* char * is inline */
	refcnt_t refcnt;
};

#define STR_STATIC_INITIALIZER(val)			\
		{					\
			.str = (val),			\
			.static_struct = true,		\
			.static_alloc = true,		\
		}

/* evaluates to a struct str *, so it can be used as a value */
#define STATIC_STR(s)					\
	({						\
		static struct str _s = {		\
			.str = (s),			\
			.static_struct = true,		\
			.static_alloc = true,		\
		};					\
		&_s;					\
	})

extern struct str *str_alloc(char *s);
extern struct str *str_alloc_static(const char *s);
extern size_t str_len(const struct str *str);
extern int str_cmp(const struct str *a, const struct str *b);
extern struct str *str_dup(const char *s);
extern struct str *str_cat(size_t n, ...);
extern struct str *str_printf(const char *fmt, ...)
	__attribute__((format (printf, 1, 2)));
extern struct str *str_vprintf(const char *fmt, va_list args);
extern void str_free(struct str *str);

static inline bool str_isstatic(struct str *x)
{
	return x->static_struct;
}

REFCNT_INLINE_FXNS(struct str, str, refcnt, str_free, str_isstatic)

#define STR_ALLOC(s)			\
	({				\
		struct str *_s;		\
		_s = str_alloc(s);	\
		ASSERT(_s);		\
		_s;			\
	})

#define STR_ALLOC_STATIC(s)		\
	({				\
		struct str *_s;		\
		_s = str_alloc_static(s);\
		ASSERT(_s);		\
		_s;			\
	})

#define STR_DUP(s)			\
	({				\
		struct str *_s;		\
		_s = str_dup(s);	\
		ASSERT(_s);		\
		_s;			\
	})

static inline const char *str_cstr(const struct str *str)
{
	if (!str)
		return NULL;
	return str->inline_alloc ? str->inline_str : str->str;
}

#endif
