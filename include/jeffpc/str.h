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
	/*
	 * Ideally, we could define the whole struct without padding bytes,
	 * but C, ABIs, and ISAs get in our way.
	 *
	 * The tricky part is that we don't want to waste any memory on
	 * structure padding.  The refcount is always 4 bytes, and the
	 * boolean flags are always a byte.  This means that we'd want the
	 * compiler to make the union (of str and inline_str) an odd number
	 * of bytes (e.g., 15, 19, 23, ...) so that the remaining 5 bytes
	 * make the size a nice multiple.
	 *
	 * We cannot simply add the packed attribute onto the union since
	 * that would generate terrible code when trying to access the ->str
	 * pointer.  Adding packed,aligned(8) or something like that
	 * generates padding.  (Actually, aligned(8) makes the union a
	 * multiple of 8 bytes creating tons of padding on 32-bit systems!)
	 *
	 * Instead of coming up with a convoluted scheme that makes the
	 * structure's usage difficult, we just suck it up and use 3 bytes
	 * of padding.  While this is inefficient from memory usage
	 * perspective, we can use this padding in the future to extend this
	 * API's functionality in the future.
	 */

	refcnt_t refcnt;

	bool static_struct:1;	/* struct str is static */
	bool static_alloc:1;	/* char * is static */
	bool inline_alloc:1;	/* char * is inline */

	uint8_t _pad0;
	uint16_t _pad1;

	union {
		const char *str;
		char inline_str[STR_INLINE_LEN + 1];
	};
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
		const char *_type_check __attribute__((__unused__)) = (s); \
		static struct str _s = {		\
			.str = (char *) (s),		\
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
