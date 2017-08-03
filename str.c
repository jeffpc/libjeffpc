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
static struct str one_char[128] = {
	['\''] = _STR_STATIC_INITIALIZER("'", 1),
	['\\'] = _STR_STATIC_INITIALIZER("\\", 1),
	['\n'] = _STR_STATIC_INITIALIZER("\n", 1),
	['\r'] = _STR_STATIC_INITIALIZER("\r", 1),
	['\t'] = _STR_STATIC_INITIALIZER("\t", 1),

	[' '] = _STR_STATIC_INITIALIZER(" ", 1),
	['!'] = _STR_STATIC_INITIALIZER("!", 1),
	['"'] = _STR_STATIC_INITIALIZER("\"", 1),
	['#'] = _STR_STATIC_INITIALIZER("#", 1),
	['$'] = _STR_STATIC_INITIALIZER("$", 1),
	['%'] = _STR_STATIC_INITIALIZER("%", 1),
	['&'] = _STR_STATIC_INITIALIZER("&", 1),
	['('] = _STR_STATIC_INITIALIZER("(", 1),
	[')'] = _STR_STATIC_INITIALIZER(")", 1),
	['*'] = _STR_STATIC_INITIALIZER("*", 1),
	['+'] = _STR_STATIC_INITIALIZER("+", 1),
	[','] = _STR_STATIC_INITIALIZER(",", 1),
	['-'] = _STR_STATIC_INITIALIZER("-", 1),
	['-'] = _STR_STATIC_INITIALIZER("-", 1),
	['.'] = _STR_STATIC_INITIALIZER(".", 1),
	['/'] = _STR_STATIC_INITIALIZER("/", 1),
	[':'] = _STR_STATIC_INITIALIZER(":", 1),
	[';'] = _STR_STATIC_INITIALIZER(";", 1),
	['<'] = _STR_STATIC_INITIALIZER("<", 1),
	['='] = _STR_STATIC_INITIALIZER("=", 1),
	['>'] = _STR_STATIC_INITIALIZER(">", 1),
	['?'] = _STR_STATIC_INITIALIZER("?", 1),
	['@'] = _STR_STATIC_INITIALIZER("@", 1),
	['['] = _STR_STATIC_INITIALIZER("[", 1),
	[']'] = _STR_STATIC_INITIALIZER("]", 1),
	['^'] = _STR_STATIC_INITIALIZER("^", 1),
	['_'] = _STR_STATIC_INITIALIZER("_", 1),
	['`'] = _STR_STATIC_INITIALIZER("`", 1),
	['{'] = _STR_STATIC_INITIALIZER("{", 1),
	['|'] = _STR_STATIC_INITIALIZER("|", 1),
	['}'] = _STR_STATIC_INITIALIZER("}", 1),
	['~'] = _STR_STATIC_INITIALIZER("~", 1),

	['0'] = _STR_STATIC_INITIALIZER("0", 1),
	['1'] = _STR_STATIC_INITIALIZER("1", 1),
	['2'] = _STR_STATIC_INITIALIZER("2", 1),
	['3'] = _STR_STATIC_INITIALIZER("3", 1),
	['4'] = _STR_STATIC_INITIALIZER("4", 1),
	['5'] = _STR_STATIC_INITIALIZER("5", 1),
	['6'] = _STR_STATIC_INITIALIZER("6", 1),
	['7'] = _STR_STATIC_INITIALIZER("7", 1),
	['8'] = _STR_STATIC_INITIALIZER("8", 1),
	['9'] = _STR_STATIC_INITIALIZER("9", 1),

	['A'] = _STR_STATIC_INITIALIZER("A", 1),
	['B'] = _STR_STATIC_INITIALIZER("B", 1),
	['C'] = _STR_STATIC_INITIALIZER("C", 1),
	['D'] = _STR_STATIC_INITIALIZER("D", 1),
	['E'] = _STR_STATIC_INITIALIZER("E", 1),
	['F'] = _STR_STATIC_INITIALIZER("F", 1),
	['G'] = _STR_STATIC_INITIALIZER("G", 1),
	['H'] = _STR_STATIC_INITIALIZER("H", 1),
	['I'] = _STR_STATIC_INITIALIZER("I", 1),
	['J'] = _STR_STATIC_INITIALIZER("J", 1),
	['K'] = _STR_STATIC_INITIALIZER("K", 1),
	['L'] = _STR_STATIC_INITIALIZER("L", 1),
	['M'] = _STR_STATIC_INITIALIZER("M", 1),
	['N'] = _STR_STATIC_INITIALIZER("N", 1),
	['O'] = _STR_STATIC_INITIALIZER("O", 1),
	['P'] = _STR_STATIC_INITIALIZER("P", 1),
	['Q'] = _STR_STATIC_INITIALIZER("Q", 1),
	['R'] = _STR_STATIC_INITIALIZER("R", 1),
	['S'] = _STR_STATIC_INITIALIZER("S", 1),
	['T'] = _STR_STATIC_INITIALIZER("T", 1),
	['U'] = _STR_STATIC_INITIALIZER("U", 1),
	['V'] = _STR_STATIC_INITIALIZER("V", 1),
	['W'] = _STR_STATIC_INITIALIZER("W", 1),
	['X'] = _STR_STATIC_INITIALIZER("X", 1),
	['Y'] = _STR_STATIC_INITIALIZER("Y", 1),
	['Z'] = _STR_STATIC_INITIALIZER("Z", 1),

	['a'] = _STR_STATIC_INITIALIZER("a", 1),
	['b'] = _STR_STATIC_INITIALIZER("b", 1),
	['c'] = _STR_STATIC_INITIALIZER("c", 1),
	['d'] = _STR_STATIC_INITIALIZER("d", 1),
	['e'] = _STR_STATIC_INITIALIZER("e", 1),
	['f'] = _STR_STATIC_INITIALIZER("f", 1),
	['g'] = _STR_STATIC_INITIALIZER("g", 1),
	['h'] = _STR_STATIC_INITIALIZER("h", 1),
	['i'] = _STR_STATIC_INITIALIZER("i", 1),
	['j'] = _STR_STATIC_INITIALIZER("j", 1),
	['k'] = _STR_STATIC_INITIALIZER("k", 1),
	['l'] = _STR_STATIC_INITIALIZER("l", 1),
	['m'] = _STR_STATIC_INITIALIZER("m", 1),
	['n'] = _STR_STATIC_INITIALIZER("n", 1),
	['o'] = _STR_STATIC_INITIALIZER("o", 1),
	['p'] = _STR_STATIC_INITIALIZER("p", 1),
	['q'] = _STR_STATIC_INITIALIZER("q", 1),
	['r'] = _STR_STATIC_INITIALIZER("r", 1),
	['s'] = _STR_STATIC_INITIALIZER("s", 1),
	['t'] = _STR_STATIC_INITIALIZER("t", 1),
	['u'] = _STR_STATIC_INITIALIZER("u", 1),
	['v'] = _STR_STATIC_INITIALIZER("v", 1),
	['w'] = _STR_STATIC_INITIALIZER("w", 1),
	['x'] = _STR_STATIC_INITIALIZER("x", 1),
	['y'] = _STR_STATIC_INITIALIZER("y", 1),
	['z'] = _STR_STATIC_INITIALIZER("z", 1),
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
	size_t totallen;
	struct str *ret;
	char *buf, *out;
	size_t *len;
	va_list ap;
	size_t i;

	if (!n)
		return NULL;

	if (n == 1) {
		va_start(ap, n);
		ret = va_arg(ap, struct str *);
		va_end(ap);
		return ret;
	}

	totallen = 0;
	len = alloca(sizeof(size_t) * n);

	va_start(ap, n);
	for (i = 0; i < n; i++) {
		struct str *str = va_arg(ap, struct str *);

		if (!str)
			continue;

		len[i] = str_get_len(str);

		totallen += len[i];
	}
	va_end(ap);

	buf = malloc(totallen + 1);
	ASSERT(buf);

	out = buf;

	va_start(ap, n);
	for (i = 0; i < n; i++) {
		struct str *str = va_arg(ap, struct str *);

		if (!str)
			continue;

		strcpy(out, str_cstr(str));

		out += len[i];

		str_putref(str);
	}
	va_end(ap);

	ret = str_alloc(buf);
	ASSERT(ret);

	return ret;
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
