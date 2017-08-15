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

#include <jeffpc/str.h>
#include <jeffpc/mem.h>
#include <jeffpc/jeffpc.h>

/* check that STR_INLINE_LEN is used properly in the struct str definition */
STATIC_ASSERT(STR_INLINE_LEN + 1 == sizeof(((struct str *) NULL)->inline_str));

/* check that STR_INLINE_LEN is not undersized */
STATIC_ASSERT(STR_INLINE_LEN + 1 >= sizeof(char *));

#define USE_STRLEN	((size_t) ~0ul)

static struct str empty_string = _STR_STATIC_INITIALIZER("", 0);

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

static struct mem_cache *str_cache;

static inline size_t str_get_len(const struct str *str)
{
	if (!str->have_len)
		return strlen(str_cstr(str));

	return (str->len[0] << 16) | (str->len[1] << 8) | str->len[2];
}

static inline void str_set_len(struct str *str, size_t len)
{
	if (len > 0xffffff) {
		str->have_len = false;
		str->len[0] = 0xff;
		str->len[1] = 0xff;
		str->len[2] = 0xff;
	} else {
		str->have_len = true;
		str->len[0] = (len >> 16) & 0xff;
		str->len[1] = (len >> 8) & 0xff;
		str->len[2] = len & 0xff;
	}
}

static void __attribute__((constructor)) init_str_subsys(void)
{
	str_cache = mem_cache_create("str-cache", sizeof(struct str), 0);
	ASSERT(!IS_ERR(str_cache));
}

static struct str *__get_preallocated(const char *s, size_t len)
{
	unsigned char first_char;

	/* NULL or non-nul terminated & zero length */
	if (!s || !len)
		return &empty_string;

	first_char = s[0];

	/* preallocated one-char long strings of 7-bit ASCII */
	if ((len == 1) &&
	    (first_char > '\0') && (first_char < '\x7f') &&
	    one_char[first_char].static_struct)
		return &one_char[first_char];

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

static struct str *__alloc(const char *s, size_t len, bool heapalloc,
			   bool mustdup)
{
	struct str *str;
	bool copy;

	/* sanity check */
	if (mustdup)
		ASSERT(!heapalloc);

	/* determine the real length of the string */
	len = s ? strnlen(s, len) : 0;

	/* check preallocated strings */
	str = __get_preallocated(s, len);
	if (str)
		goto out;

	/* can we inline it? */
	copy = __inlinable(len);

	/* we'll be storing a pointer - strdup as necessary */
	if (!copy && mustdup) {
		char *tmp;

		tmp = malloc(len + 1);
		if (!tmp)
			goto out;

		__copy(tmp, s, len);

		/* we're now using the heap */
		heapalloc = true;
		s = tmp;
	}

	str = mem_cache_alloc(str_cache);
	if (!str)
		goto out;

	refcnt_init(&str->refcnt, 1);
	str->static_struct = false;
	str->static_alloc = copy || !heapalloc;
	str->inline_alloc = copy;
	str_set_len(str, len);

	if (copy) {
		__copy(str->inline_str, s, len);

		if (heapalloc)
			free((char *) s);
	} else {
		str->str = s;
	}

	return str;

out:
	if (heapalloc)
		free((char *) s);

	return str;
}

/*
 * Passed in str cannot be freed, and it must be dup'd.  (E.g., it could be
 * a string on the stack.)
 */
struct str *str_dup(const char *s)
{
	return __alloc(s, USE_STRLEN, false, true);
}

struct str *str_dup_len(const char *s, size_t len)
{
	return __alloc(s, len, false, true);
}

/* Passed in str must be freed. */
struct str *str_alloc(char *s)
{
	return __alloc(s, USE_STRLEN, true, false);
}

/*
 * Passed in str cannot be freed, and it doesn't have to be dup'd.  (E.g.,
 * it could be a string in .rodata.)
 */
struct str *str_alloc_static(const char *s)
{
	return __alloc(s, USE_STRLEN, false, false);
}

size_t str_len(const struct str *s)
{
	return str_get_len(s);
}

int str_cmp(const struct str *a, const struct str *b)
{
	return strcmp(str_cstr(a), str_cstr(b));
}

struct str *str_cat(size_t n, ...)
{
	const size_t nargs = n;
	size_t totallen;
	struct str *ret;
	char *buf, *out;
	size_t *len;
	va_list ap;
	size_t i;

	if (!nargs)
		return NULL;

	if (nargs == 1) {
		va_start(ap, n);
		ret = va_arg(ap, struct str *);
		va_end(ap);
		return ret;
	}

	totallen = 0;
	len = alloca(sizeof(size_t) * n);

	va_start(ap, n);
	for (i = 0; i < nargs; i++) {
		struct str *str = va_arg(ap, struct str *);

		len[i] = str ? str_get_len(str) : 0;

		totallen += len[i];
	}
	va_end(ap);

	buf = malloc(totallen + 1);
	ASSERT(buf);

	buf[0] = '\0';

	out = buf;

	va_start(ap, n);
	for (i = 0; i < nargs; i++) {
		struct str *str = va_arg(ap, struct str *);

		if (!str)
			continue;

		strcpy(out, str_cstr(str));

		out += len[i];

		str_putref(str);
	}
	va_end(ap);

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

void str_free(struct str *str)
{
	ASSERT(str);
	ASSERT3U(refcnt_read(&str->refcnt), ==, 0);

	if (!str->inline_alloc && !str->static_alloc)
		free((char *) str->str);
	mem_cache_free(str_cache, str);
}

struct str *str_empty_string(void)
{
	return &empty_string;
}
