/*
 * Copyright (c) 2015-2017 Josef 'Jeff' Sipek <jeffpc@josefsipek.net>
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

#include <stdio.h>
#include <ctype.h>

#include <jeffpc/sexpr.h>

#include "sexpr_impl.h"

static struct str *do_sexpr_dump(struct val *lv, bool raw, bool wrap);

static char *escape_str(const char *in)
{
	char *out, *tmp;
	const char *s;
	size_t outlen;

	outlen = strlen(in);

	for (s = in; *s; s++) {
		char c = *s;

		switch (c) {
			case '\n':
			case '\t':
			case '\r':
			case '\b':
			case '\f':
			case '"':
				/* "\n", "\t", ... */
				outlen++;
				break;
		}
	}

	out = malloc(outlen + 1);
	if (!out)
		return NULL;

	for (s = in, tmp = out; *s; s++, tmp++) {
		unsigned char c = *s;

		switch (c) {
			case '\n':
				strcpy(tmp, "\\n");
				tmp++;
				break;
			case '\t':
				strcpy(tmp, "\\t");
				tmp++;
				break;
			case '\r':
				strcpy(tmp, "\\r");
				tmp++;
				break;
			case '\b':
				strcpy(tmp, "\\b");
				tmp++;
				break;
			case '\f':
				strcpy(tmp, "\\f");
				tmp++;
				break;
			case '"':
				strcpy(tmp, "\\\"");
				tmp++;
				break;
			default:
				*tmp = c;
				break;
		}
	}

	*tmp = '\0';

	return out;
}

static struct str *dump_atom(struct val *lv)
{
	static struct str dquote = STR_STATIC_CHAR_INITIALIZER('"');
	static struct str null = STR_STATIC_INITIALIZER("#n");
	static struct str poundt = STR_STATIC_INITIALIZER("#t");
	static struct str poundf = STR_STATIC_INITIALIZER("#f");
	struct str *tmp;

	switch (lv->type) {
		case VT_SYM:
			return str_dup(val_cstr(lv));
		case VT_STR:
			tmp = str_alloc(escape_str(val_cstr(lv)));
			if (IS_ERR(tmp))
				return tmp;

			return str_cat(3, &dquote, tmp, &dquote);
		case VT_NULL:
			return &null;
		case VT_BOOL:
			return lv->b ? &poundt : &poundf;
		case VT_CHAR:
			if (isprint(lv->i))
				return str_printf("#\\%c", (char) lv->i);
			else
				return str_printf("#\\u%04"PRIX64, lv->i);
		case VT_INT:
			return str_printf("%"PRIu64, lv->i);
		case VT_CONS:
			panic("%s: VT_CONS is not an atom", __func__);
		case VT_BLOB:
			return ERR_PTR(-ENOTSUP);
	}

	panic("%s: unknown val type: %u", __func__, lv->type);
}

static inline int dump_cons_parts(struct val *head, struct str **hstr,
				  struct val *tail, struct str **tstr,
				  bool raw)
{
	struct str *h, *t;

	/* pacify gcc */
	*hstr = NULL;
	*tstr = NULL;

	/* the head needs to always be wrapped */
	h = do_sexpr_dump(head, raw, true);
	if (IS_ERR(h))
		return PTR_ERR(h);

	/* the tail needs to be wrapped only when dumping raw */
	t = do_sexpr_dump(tail, raw, raw);
	if (IS_ERR(t)) {
		str_putref(h);
		return PTR_ERR(t);
	}

	*hstr = h;
	*tstr = t;

	return 0;
}

static struct str *dump_cons_pretty_quote(struct val *lv)
{
	static struct str squote = STR_STATIC_CHAR_INITIALIZER('\'');
	struct str *tmp;

	tmp = do_sexpr_dump(lv, false, false);
	if (IS_ERR(tmp))
		return tmp;

	return str_cat(2, &squote, tmp);
}

static struct str *dump_cons_pretty(struct val *lv, bool wrap)
{
	static struct str oparen = STR_STATIC_CHAR_INITIALIZER('(');
	static struct str cparen = STR_STATIC_CHAR_INITIALIZER(')');
	static struct str dot = STR_STATIC_INITIALIZER(" . ");
	static struct str space = STR_STATIC_INITIALIZER(" ");
	struct val *head = lv->cons.head;
	struct val *tail = lv->cons.tail;
	struct str *hstr;
	struct str *tstr;
	struct str *tmp;
	int ret;

	ASSERT(!sexpr_is_null(lv));
	ASSERT3U(lv->type, ==, VT_CONS);

	if (head && (head->type == VT_SYM) && !strcmp(val_cstr(head), "quote"))
		/*
		 * quote . tail		-> 'tail
		 *
		 * note: we don't wrap
		 */
		return dump_cons_pretty_quote(tail);

	ret = dump_cons_parts(head, &hstr, tail, &tstr, false);
	if (ret)
		return ERR_PTR(ret);

	if (sexpr_is_null(tail)) {
		/* head . ()		-> head */
		str_putref(tstr);
		tmp = hstr;
	} else if (tail->type == VT_CONS) {
		/* head . (x . y)	-> head x . y */
		tmp = str_cat(3, hstr, &space, tstr);
	} else {
		/* head . tail		-> head . tail */
		tmp = str_cat(3, hstr, &dot, tstr);
	}

	return wrap ? str_cat(3, &oparen, tmp, &cparen) : tmp;
}

static struct str *dump_cons_raw(struct val *lv)
{
	static struct str oparen = STR_STATIC_CHAR_INITIALIZER('(');
	static struct str cparen = STR_STATIC_CHAR_INITIALIZER(')');
	static struct str dot = STR_STATIC_INITIALIZER(" . ");
	struct str *hstr;
	struct str *tstr;
	int ret;

	ret = dump_cons_parts(lv->cons.head, &hstr, lv->cons.tail, &tstr, true);
	if (ret)
		return ERR_PTR(ret);

	return str_cat(5, &oparen, hstr, &dot, tstr, &cparen);
}

static struct str *do_sexpr_dump(struct val *lv, bool raw, bool wrap)
{
	static struct str empty = STR_STATIC_INITIALIZER("()");

	if (sexpr_is_null(lv))
		return &empty;

	switch (lv->type) {
		case VT_SYM:
		case VT_STR:
		case VT_NULL:
		case VT_BOOL:
		case VT_CHAR:
		case VT_INT:
		case VT_BLOB:
			return dump_atom(lv);
		case VT_CONS:
			if (raw)
				return dump_cons_raw(lv);

			return dump_cons_pretty(lv, wrap);
	}

	panic("%s: unknown val type: %u", __func__, lv->type);
}

struct str *sexpr_dump(struct val *lv, bool raw)
{
	return do_sexpr_dump(lv, raw, true);
}

int sexpr_dump_file(FILE *out, struct val *lv, bool raw)
{
	struct str *tmp;

	tmp = sexpr_dump(lv, raw);
	if (IS_ERR(tmp))
		return PTR_ERR(tmp);

	fprintf(out, "%s", str_cstr(tmp));

	str_putref(tmp);

	return 0;
}
