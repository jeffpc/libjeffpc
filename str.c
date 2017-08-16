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

#include <stdlib.h>
#include <stdbool.h>
#include <alloca.h>

#include <jeffpc/mem.h>
#include <jeffpc/jeffpc.h>

#include "val_impl.h"

/* check that STR_INLINE_LEN is used properly in the struct definition */
STATIC_ASSERT(STR_INLINE_LEN + 1 == sizeof(((struct str *) NULL)->val.str_inline));

/* check that STR_INLINE_LEN is not undersized */
STATIC_ASSERT(STR_INLINE_LEN + 1 >= sizeof(char *));

/* check that struct str is just a type alias for struct val */
STATIC_ASSERT(sizeof(struct str) == sizeof(struct val));
STATIC_ASSERT(offsetof(struct str, val) == 0);

/* same as above but for struct sym */
STATIC_ASSERT(sizeof(struct sym) == sizeof(struct val));
STATIC_ASSERT(offsetof(struct sym, val) == 0);

#define USE_STRLEN	((size_t) ~0ul)

static struct str empty_string = _STATIC_STR_INITIALIZER(VT_STR, "");
static struct sym empty_symbol = _STATIC_STR_INITIALIZER(VT_SYM, "");

/* one 7-bit ASCII character long strings */
static struct str one_char[128] = {
#define STATIC_CHAR1(start) \
	[start] = STR_STATIC_CHAR_INITIALIZER(start)
#define STATIC_CHAR2(start) \
	STATIC_CHAR1(start), STATIC_CHAR1(start + 1)
#define STATIC_CHAR4(start) \
	STATIC_CHAR2(start), STATIC_CHAR2(start + 2)
#define STATIC_CHAR8(start) \
	STATIC_CHAR4(start), STATIC_CHAR4(start + 4)
#define STATIC_CHAR16(start) \
	STATIC_CHAR8(start), STATIC_CHAR8(start + 8)
#define STATIC_CHAR32(start) \
	STATIC_CHAR16(start), STATIC_CHAR16(start + 16)

	/* Note: 0 is not initialized */
	STATIC_CHAR1(1),	/*       1 */
	STATIC_CHAR2(2),	/*  2....3 */
	STATIC_CHAR4(4),	/*  4....7 */
	STATIC_CHAR8(8),	/*  8...15 */
	STATIC_CHAR16(16),	/* 16...31 */
	STATIC_CHAR32(32),	/* 32...63 */
	STATIC_CHAR32(64),	/* 64...95  */
	STATIC_CHAR32(96)	/* 96..127 */
};

static inline size_t get_len(const struct val *val)
{
	return strlen(val_cstr(val));
}

static inline void set_len(struct val *str, size_t len)
{
	/* TODO: stash length in struct val if possible */
}

static struct val *__get_preallocated(enum val_type type, const char *s,
				      size_t len)
{
	unsigned char first_char;

	/* NULL or non-nul terminated & zero length */
	if (!s || !len)
		return (type == VT_STR) ? &empty_string.val : &empty_symbol.val;

	if (type != VT_STR)
		return NULL;

	first_char = s[0];

	/* preallocated one-char long strings of 7-bit ASCII */
	if ((len == 1) &&
	    (first_char > '\0') && (first_char < '\x7f') &&
	    one_char[first_char].val.static_struct)
		return &one_char[first_char].val;

	/* nothing pre-allocated */
	return NULL;
}

static bool __inlinable(size_t len)
{
	return (len <= STR_INLINE_LEN);
}

static inline void __copy(char *dest, const char *s, size_t len)
{
	memcpy(dest, s, len);
	dest[len] = '\0';
}

static struct val *__alloc(enum val_type type, const char *s, size_t len,
			   bool heapalloc, bool mustdup)
{
	struct val *val;
	bool copy;

	ASSERT((type == VT_STR) || (type == VT_SYM));

	/* sanity check */
	if (mustdup)
		ASSERT(!heapalloc);

	/* determine the real length of the string */
	len = s ? strnlen(s, len) : 0;

	/* check preallocated strings */
	val = __get_preallocated(type, s, len);
	if (val)
		goto out;

	/* can we inline it? */
	copy = __inlinable(len);

	/* we'll be storing a pointer - strdup as necessary */
	if (!copy && mustdup) {
		char *tmp;

		tmp = malloc(len + 1);
		if (!tmp) {
			val = ERR_PTR(-ENOMEM);
			goto out;
		}

		__copy(tmp, s, len);

		/* we're now using the heap */
		heapalloc = true;
		s = tmp;
	}

	val = __val_alloc(type);
	if (IS_ERR(val))
		goto out;

	val->static_alloc = copy || !heapalloc;
	val->inline_alloc = copy;
	set_len(val, len);

	if (copy) {
		__copy(val->_set_str_inline, s, len);

		if (heapalloc)
			free((char *) s);
	} else {
		val->_set_str_ptr = s;
	}

	return val;

out:
	if (heapalloc)
		free((char *) s);

	return val;
}

struct val *_strsym_dup(const char *s, enum val_type type)
{
	return __alloc(type, s, USE_STRLEN, false, true);
}

struct val *_strsym_dup_len(const char *s, size_t len, enum val_type type)
{
	return __alloc(type, s, len, false, true);
}

struct val *_strsym_alloc(char *s, enum val_type type)
{
	return __alloc(type, s, USE_STRLEN, true, false);
}

struct val *_strsym_alloc_static(const char *s, enum val_type type)
{
	return __alloc(type, s, USE_STRLEN, false, false);
}

size_t _strsym_len(const struct val *s)
{
	return get_len(s);
}

int _strsym_cmp(const struct val *a, const struct val *b)
{
	return strcmp(val_cstr(a), val_cstr(b));
}

struct str *str_cat(size_t n, ...)
{
	size_t totallen;
	char *buf, *out;
	struct val **vals;
	size_t *len;
	va_list ap;
	size_t i;

	if (!n)
		return NULL;

	if (n == 1) {
		struct val *val;
		struct str *ret;

		va_start(ap, n);
		val = va_arg(ap, struct val *);
		va_end(ap);

		if (val->type == VT_STR)
			return val_cast_to_str(val);

		ret = STR_DUP(val_cstr(val));
		val_putref(val);
		return ret;
	}

	totallen = 0;
	len = alloca(sizeof(size_t) * n);
	vals = alloca(sizeof(struct val *) * n);

	va_start(ap, n);
	for (i = 0; i < n; i++) {
		struct val *val = va_arg(ap, struct val *);

		if (!val) {
			len[i] = 0;
			vals[i] = NULL;
		} else {
			len[i] = get_len(val);
			vals[i] = val;

			totallen += len[i];
		}
	}
	va_end(ap);

	buf = malloc(totallen + 1);
	ASSERT(buf);

	buf[0] = '\0';

	out = buf;

	for (i = 0; i < n; i++) {
		struct val *val = vals[i];

		if (!val)
			continue;

		strcpy(out, val_cstr(val));

		out += len[i];

		val_putref(val);
	}

	return STR_ALLOC(buf);
}

struct str *str_vprintf(const char *fmt, va_list args)
{
	char *tmp;
	int ret;

	ret = vasprintf(&tmp, fmt, args);
	if (ret < 0)
		return ERR_PTR(-errno);
	if (ret == 0)
		return NULL;

	return STR_ALLOC(tmp);
}

struct str *str_printf(const char *fmt, ...)
{
	struct str *ret;
	va_list args;

	va_start(args, fmt);
	ret = str_vprintf(fmt, args);
	va_end(args);

	return ret;
}

struct str *str_empty_string(void)
{
	return &empty_string;
}
