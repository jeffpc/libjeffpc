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

/*
 * Changing this value by...
 *    4 will maintain the amount of padding on 32-bit systems.
 *    8 will maintain the amount of padding on both 32-bit and 64-bit systems.
 */
#define STR_INLINE_LEN	15

struct str {
	refcnt_t refcnt;

	bool static_struct:1;	/* struct str is static */
	bool static_alloc:1;	/* char * is static */
	bool inline_alloc:1;	/* char * is inline */
	bool have_len:1;	/* length member is valid */

	/*
	 * Keep track of a 24-bit length of the string.  While we could use
	 * a size_t to represent all possible lengths, we use only 24 bits
	 * to represents strings from 0 to ~16MB in size.  This covers the
	 * vast majority of them.  Strings that have length less than or
	 * equal to 16777215 (0xffffff) bytes save the length in the len
	 * member and set have_len to true.  Strings that are longer set
	 * have_len to false and ignore the len member.
	 */
	uint8_t len[3];

	union {
		const char *str;
		char inline_str[STR_INLINE_LEN + 1];
	};
};

#define _STR_STATIC_INITIALIZER(val, l)			\
		{					\
			.str = (val),			\
			.len = {			\
				((l) >> 16) & 0xff,	\
				((l) >> 8) & 0xff,	\
				(l) & 0xff,		\
			},				\
			.have_len = ((l) <= 0xffffff),	\
			.static_struct = true,		\
			.static_alloc = true,		\
		}

#define STR_STATIC_INITIALIZER(val)			\
		_STR_STATIC_INITIALIZER((val),		\
					__builtin_constant_p(val) ? strlen(val) : ~0ul)

/* evaluates to a struct str *, so it can be used as a value */
#define STATIC_STR(s)					\
	({						\
		static struct str _s = STR_STATIC_INITIALIZER(s); \
		&_s;					\
	})

extern struct str *str_alloc(char *s);
extern struct str *str_alloc_static(const char *s);
extern size_t str_len(const struct str *str);
extern int str_cmp(const struct str *a, const struct str *b);
extern struct str *str_dup(const char *s);
extern struct str *str_dup_len(const char *s, size_t len);
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
