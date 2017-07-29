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

static struct str empty_string = STR_STATIC_INITIALIZER("");
static struct str one_char[128] = {
	['\''] = STR_STATIC_INITIALIZER("'"),
	['\\'] = STR_STATIC_INITIALIZER("\\"),
	['\n'] = STR_STATIC_INITIALIZER("\n"),
	['\r'] = STR_STATIC_INITIALIZER("\r"),
	['\t'] = STR_STATIC_INITIALIZER("\t"),

	[' '] = STR_STATIC_INITIALIZER(" "),
	['!'] = STR_STATIC_INITIALIZER("!"),
	['"'] = STR_STATIC_INITIALIZER("\""),
	['#'] = STR_STATIC_INITIALIZER("#"),
	['$'] = STR_STATIC_INITIALIZER("$"),
	['%'] = STR_STATIC_INITIALIZER("%"),
	['&'] = STR_STATIC_INITIALIZER("&"),
	['('] = STR_STATIC_INITIALIZER("("),
	[')'] = STR_STATIC_INITIALIZER(")"),
	['*'] = STR_STATIC_INITIALIZER("*"),
	['+'] = STR_STATIC_INITIALIZER("+"),
	[','] = STR_STATIC_INITIALIZER(","),
	['-'] = STR_STATIC_INITIALIZER("-"),
	['-'] = STR_STATIC_INITIALIZER("-"),
	['.'] = STR_STATIC_INITIALIZER("."),
	['/'] = STR_STATIC_INITIALIZER("/"),
	[':'] = STR_STATIC_INITIALIZER(":"),
	[';'] = STR_STATIC_INITIALIZER(";"),
	['<'] = STR_STATIC_INITIALIZER("<"),
	['='] = STR_STATIC_INITIALIZER("="),
	['>'] = STR_STATIC_INITIALIZER(">"),
	['?'] = STR_STATIC_INITIALIZER("?"),
	['@'] = STR_STATIC_INITIALIZER("@"),
	['['] = STR_STATIC_INITIALIZER("["),
	[']'] = STR_STATIC_INITIALIZER("]"),
	['^'] = STR_STATIC_INITIALIZER("^"),
	['_'] = STR_STATIC_INITIALIZER("_"),
	['`'] = STR_STATIC_INITIALIZER("`"),
	['{'] = STR_STATIC_INITIALIZER("{"),
	['|'] = STR_STATIC_INITIALIZER("|"),
	['}'] = STR_STATIC_INITIALIZER("}"),
	['~'] = STR_STATIC_INITIALIZER("~"),

	['0'] = STR_STATIC_INITIALIZER("0"),
	['1'] = STR_STATIC_INITIALIZER("1"),
	['2'] = STR_STATIC_INITIALIZER("2"),
	['3'] = STR_STATIC_INITIALIZER("3"),
	['4'] = STR_STATIC_INITIALIZER("4"),
	['5'] = STR_STATIC_INITIALIZER("5"),
	['6'] = STR_STATIC_INITIALIZER("6"),
	['7'] = STR_STATIC_INITIALIZER("7"),
	['8'] = STR_STATIC_INITIALIZER("8"),
	['9'] = STR_STATIC_INITIALIZER("9"),

	['A'] = STR_STATIC_INITIALIZER("A"),
	['B'] = STR_STATIC_INITIALIZER("B"),
	['C'] = STR_STATIC_INITIALIZER("C"),
	['D'] = STR_STATIC_INITIALIZER("D"),
	['E'] = STR_STATIC_INITIALIZER("E"),
	['F'] = STR_STATIC_INITIALIZER("F"),
	['G'] = STR_STATIC_INITIALIZER("G"),
	['H'] = STR_STATIC_INITIALIZER("H"),
	['I'] = STR_STATIC_INITIALIZER("I"),
	['J'] = STR_STATIC_INITIALIZER("J"),
	['K'] = STR_STATIC_INITIALIZER("K"),
	['L'] = STR_STATIC_INITIALIZER("L"),
	['M'] = STR_STATIC_INITIALIZER("M"),
	['N'] = STR_STATIC_INITIALIZER("N"),
	['O'] = STR_STATIC_INITIALIZER("O"),
	['P'] = STR_STATIC_INITIALIZER("P"),
	['Q'] = STR_STATIC_INITIALIZER("Q"),
	['R'] = STR_STATIC_INITIALIZER("R"),
	['S'] = STR_STATIC_INITIALIZER("S"),
	['T'] = STR_STATIC_INITIALIZER("T"),
	['U'] = STR_STATIC_INITIALIZER("U"),
	['V'] = STR_STATIC_INITIALIZER("V"),
	['W'] = STR_STATIC_INITIALIZER("W"),
	['X'] = STR_STATIC_INITIALIZER("X"),
	['Y'] = STR_STATIC_INITIALIZER("Y"),
	['Z'] = STR_STATIC_INITIALIZER("Z"),

	['a'] = STR_STATIC_INITIALIZER("a"),
	['b'] = STR_STATIC_INITIALIZER("b"),
	['c'] = STR_STATIC_INITIALIZER("c"),
	['d'] = STR_STATIC_INITIALIZER("d"),
	['e'] = STR_STATIC_INITIALIZER("e"),
	['f'] = STR_STATIC_INITIALIZER("f"),
	['g'] = STR_STATIC_INITIALIZER("g"),
	['h'] = STR_STATIC_INITIALIZER("h"),
	['i'] = STR_STATIC_INITIALIZER("i"),
	['j'] = STR_STATIC_INITIALIZER("j"),
	['k'] = STR_STATIC_INITIALIZER("k"),
	['l'] = STR_STATIC_INITIALIZER("l"),
	['m'] = STR_STATIC_INITIALIZER("m"),
	['n'] = STR_STATIC_INITIALIZER("n"),
	['o'] = STR_STATIC_INITIALIZER("o"),
	['p'] = STR_STATIC_INITIALIZER("p"),
	['q'] = STR_STATIC_INITIALIZER("q"),
	['r'] = STR_STATIC_INITIALIZER("r"),
	['s'] = STR_STATIC_INITIALIZER("s"),
	['t'] = STR_STATIC_INITIALIZER("t"),
	['u'] = STR_STATIC_INITIALIZER("u"),
	['v'] = STR_STATIC_INITIALIZER("v"),
	['w'] = STR_STATIC_INITIALIZER("w"),
	['x'] = STR_STATIC_INITIALIZER("x"),
	['y'] = STR_STATIC_INITIALIZER("y"),
	['z'] = STR_STATIC_INITIALIZER("z"),
};

static struct mem_cache *str_cache;

static void __attribute__((constructor)) init_str_subsys(void)
{
	str_cache = mem_cache_create("str-cache", sizeof(struct str), 0);
	ASSERT(!IS_ERR(str_cache));
}

static struct str *__get_preallocated(const char *s)
{
	unsigned char first_char;

	/* NULL or empty string */
	if (!s || (s[0] == '\0'))
		return &empty_string;

	first_char = s[0];

	/* preallocated one-char long strings of 7-bit ASCII */
	if ((first_char > '\0') && (first_char < '\x7f') &&
	    (s[1] == '\0') && one_char[first_char].static_struct)
		return &one_char[first_char];

	/* nothing pre-allocated */
	return NULL;
}

static bool __inlinable(char *s)
{
	return strlen(s) <= STR_INLINE_LEN;
}

static struct str *__alloc(char *s, bool heapalloc, bool mustdup)
{
	struct str *str;
	bool copy;

	/* sanity check */
	if (mustdup)
		ASSERT(!heapalloc);

	/* check preallocated strings */
	str = __get_preallocated(s);
	if (str)
		goto out;

	/* can we inline it? */
	copy = __inlinable(s);

	/* we'll be storing a pointer - strdup as necessary */
	if (!copy && mustdup) {
		s = strdup(s);
		if (!s)
			goto out;

		/* we're now using the heap */
		heapalloc = true;
	}

	str = mem_cache_alloc(str_cache);
	if (!str)
		goto out;

	refcnt_init(&str->refcnt, 1);
	str->static_struct = false;
	str->static_alloc = copy || !heapalloc;
	str->inline_alloc = copy;

	if (copy) {
		strcpy(str->inline_str, s);
		if (heapalloc)
			free(s);
	} else {
		str->str = s;
	}

	return str;

out:
	if (heapalloc)
		free(s);

	return str;
}

/*
 * Passed in str cannot be freed, and it must be dup'd.  (E.g., it could be
 * a string on the stack.)
 */
struct str *str_dup(const char *s)
{
	return __alloc((char *) s, false, true);
}

/* Passed in str must be freed. */
struct str *str_alloc(char *s)
{
	return __alloc(s, true, false);
}

/*
 * Passed in str cannot be freed, and it doesn't have to be dup'd.  (E.g.,
 * it could be a string in .rodata.)
 */
struct str *str_alloc_static(const char *s)
{
	return __alloc((char *) s, false, false);
}

size_t str_len(const struct str *s)
{
	return strlen(str_cstr(s));
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

		len[i] = strlen(str_cstr(str));

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
		free(str->str);
	mem_cache_free(str_cache, str);
}
