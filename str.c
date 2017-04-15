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
	[' '] = STR_STATIC_INITIALIZER(" "),
};

static struct mem_cache *str_cache;

static void __attribute__((constructor)) init_str_subsys(void)
{
	str_cache = mem_cache_create("str-cache", sizeof(struct str), 0);
	ASSERT(!IS_ERR(str_cache));
}

static struct str *__alloc(char *s, bool copy)
{
	struct str *str;

	str = mem_cache_alloc(str_cache);
	if (!str)
		return NULL;

	refcnt_init(&str->refcnt, 1);
	str->static_alloc = false;
	str->inline_alloc = copy;

	if (copy)
		strcpy(str->inline_str, s);
	else
		str->str = s;

	return str;
}

struct str *str_dup(const char *s)
{
	unsigned char first_char;

	if (!s)
		return &empty_string;

	first_char = s[0];

	/* preallocated one-char long strings of 7-bit ASCII */
	if ((first_char > '\0') && (first_char < '\x7f') &&
	    (s[1] == '\0') && one_char[first_char].static_alloc)
		return &one_char[first_char];

	if (strlen(s) <= STR_INLINE_LEN)
		return __alloc((char *) s, true);

	return str_alloc(strdup(s));
}

struct str *str_alloc(char *s)
{
	return __alloc(s, false);
}

size_t str_len(const struct str *s)
{
	return strlen(str_cstr(s));
}

int str_cmp(const struct str *a, const struct str *b)
{
	return strcmp(str_cstr(a), str_cstr(b));
}

struct str *str_cat(int n, ...)
{
	size_t totallen;
	struct str *ret;
	char *buf, *out;
	size_t *len;
	va_list ap;
	int i;

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
	if (!str)
		return;

	ASSERT3U(refcnt_read(&str->refcnt), ==, 0);

	if (!str->inline_alloc)
		free(str->str);
	mem_cache_free(str_cache, str);
}
