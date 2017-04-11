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

#define STR_INLINE_LEN	14

struct str {
	char *str;
	refcnt_t refcnt;
	bool static_alloc:1;
	char inline_str[STR_INLINE_LEN + 1];
};

#define STR_STATIC_INITIALIZER(val)			\
		{					\
			.str = (val),			\
			.static_alloc = true,		\
		}

extern struct str *str_alloc(char *s);
extern size_t str_len(const struct str *str);
extern int str_cmp(const struct str *a, const struct str *b);
extern struct str *str_dup(const char *s);
extern struct str *str_cat(int n, ...);
extern struct str *str_printf(const char *fmt, ...)
	__attribute__((format (printf, 1, 2)));
extern struct str *str_vprintf(const char *fmt, va_list args);
extern void str_free(struct str *str);

static inline bool str_isstatic(struct str *x)
{
	return x->static_alloc;
}

REFCNT_INLINE_FXNS(struct str, str, refcnt, str_free, str_isstatic)

#define STR_ALLOC(s)			\
	({				\
		struct str *_s;		\
		_s = str_alloc(s);	\
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
	return str ? str->str : NULL;
}

#endif
