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
 * sizeof(struct str) == 24:
 *	18 for inline string
 *	1 for inline string nul-terminator
 *	1 for flags
 *	4 for refcount
 *
 * Changing this value by 4 will maintain zero padding on 32-bit systems.
 * Changing this value by 8 will maintain zero padding on both 32-bit and
 * 64-bit systems.
 */
#define STR_INLINE_LEN	18

struct str {
	/*
	 * Ideally, we could define the whole struct as:
	 *
	 *	struct str {
	 *		union {
	 *			char *str;
	 *			char inline_str[STR_INLINE_LEN + 1];
	 *		};
	 *		bool foo:1;
	 *		bool bar:1;
	 *		refcnt_t refcnt;
	 *	};
	 *
	 * But C, ABIs, and ISAs get in our way.
	 *
	 * We have three distinct members we care about:
	 *
	 *   - str pointer or inline_str array
	 *   - a number of bool flags
	 *   - a refcount
	 *
	 * The tricky part is that we don't want to waste any memory on
	 * structure padding.  The refcount is always 4 bytes, and the
	 * boolean flags are always a byte.  This means that we'd want the
	 * compiler to make the union an odd number of bytes (e.g., 15, 19,
	 * 23, ...) so that the remaining 5 bytes make the size a nice
	 * multiple.
	 *
	 * We cannot simply add the packed attribute onto the union since
	 * that would generate terrible code when trying to access the ->str
	 * pointer.  Adding packed,aligned(8) or something like that
	 * generates padding.  (Actually, aligned(8) makes the union a
	 * multiple of 8 bytes creating tons of padding on 32-bit systems!)
	 *
	 * For example, let's assume that STR_INLINE_LEN is 18.  With a byte
	 * for the nul-terminator, we want inline_str to be 19 bytes in
	 * size, immediately followed by the one byte for the boolean flags.
	 * The easiest way would be to add a third item to the union - one
	 * with 19 bytes of padding followed by the flags.  This would round
	 * out the union to 20 bytes, which should make the compiler not add
	 * any padding.  In other words:
	 *
	 *	struct str {
	 *		union {
	 *			char *str;
	 *			char inline_str[STR_INLINE_LEN + 1];
	 *			struct {
	 *				char _pad[STR_INLINE_LEN + 1];
	 *				bool foo:1;
	 *				bool bar:1;
	 *			};
	 *		};
	 *		refcnt_t refcnt;
	 *	};
	 *
	 * Unfortunately, this idea runs into trouble when we try to use
	 * designated initializers (e.g., in STR_STATIC_INITIALIZER) that
	 * set both str and a flag.  The flag initialization forces _pad to
	 * be zeroed out, which in essence nukes the str pointer we tried to
	 * store.
	 *
	 * To get around this, we can move the str pointer into the same
	 * struct as the flags and shrink the padding.  (We never initialize
	 * flags and inline string in a designated initializer.)  This
	 * produces a nicely packed structure on 32-bit systems where
	 * structures and unions get padded to multiples of 4 bytes.
	 *
	 * On 64-bit systems, where structures and unions are padded to
	 * multiples of 8 bytes, we end up with the inner struct getting
	 * padded to 24 bytes (assuming STR_INLINE_LEN is still 18), then
	 * the refcount brings up the size to 28 bytes and the outer
	 * structure adds 4 bytes of padding making the total size 32 bytes.
	 * If we move the refcount inside, we end up with a nicely packed
	 * structure on both 32-bit and 64-bit systems with zero padding and
	 * size of 24 bytes.
	 *
	 * This is why this structure is defined in such a strage way.
	 */
	union {
		char inline_str[STR_INLINE_LEN + 1];
		struct {
			const char *str;
			char _pad[STR_INLINE_LEN + 1 - sizeof(char *)];
			bool static_struct:1;	/* struct str is static */
			bool static_alloc:1;	/* char * is static */
			bool inline_alloc:1;	/* char * is inline */
			refcnt_t refcnt;
		};
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
