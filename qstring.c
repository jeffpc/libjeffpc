/*
 * Copyright (c) 2014-2018 Josef 'Jeff' Sipek <jeffpc@josefsipek.net>
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

#include <jeffpc/qstring.h>
#include <jeffpc/urldecode.h>
#include <jeffpc/error.h>

/*
 * The query string is a sequence of name-value pairs.  Each name is
 * separated from the value with a '=', and each pair is separated by '&'.
 * We parse the input string, and produce a nvlist key-value pair for each
 * of the input pairs.  We map each input pair based on these rules:
 *
 *	foo=bar		->	{ "foo" : "bar" }
 *	foo=		->	{ "foo" : "" }
 *	foo		->	{ "foo" : null }
 *	=bar		->	{ "" : "bar" }
 */

enum qs_state {
	QS_STATE_NAME,
	QS_STATE_VAL,
};

static int insert(struct nvlist *nvl, const char *namestart, size_t namelen,
		  const char *valstart, size_t vallen)
{
	char name[namelen + 1];
	ssize_t newlen;

	/* decode & nul-terminate the name */
	newlen = urldecode(namestart, namelen, name);
	if (newlen < 0)
		return newlen;

	name[newlen] = '\0';

	if (!valstart) {
		/* we want a null value */
		return nvl_set_null(nvl, name);
	} else {
		/* we want a string value (possibly empty string) */
		struct str *val;

		if (!vallen)
			val = str_empty_string();
		else
			val = urldecode_str(valstart, vallen);

		if (IS_ERR(val))
			return PTR_ERR(val);

		return nvl_set_str(nvl, name, val);
	}
}

int qstring_parse_len(struct nvlist *nvl, const char *qs, size_t len)
{
	const char *cur, *end;
	const char *name;
	const char *val;
	size_t name_len;
	enum qs_state state;

	if (!nvl)
		return -EINVAL;

	if (!len && !qs)
		return 0;

	if (!qs)
		return -EINVAL;

	end = qs + len;
	cur = qs;

	state = QS_STATE_NAME;

	name = qs;
	name_len = 0;
	val = NULL;

	for (; end > cur; cur++) {
		char c = *cur;

		if (state == QS_STATE_NAME) {
			if (c == '=') {
				/* end of name */
				name_len = cur - name;
				val = cur + 1;
				state = QS_STATE_VAL;
			} else if (c == '&') {
				/* end of name; no value */
				insert(nvl, name, cur - name, NULL, 0);

				name = cur + 1;
				state = QS_STATE_NAME; /* no change */
			}
		} else if (state == QS_STATE_VAL) {
			if (c == '&') {
				/* end of value */
				insert(nvl, name, name_len, val, cur - val);

				name = cur + 1;
				state = QS_STATE_NAME;
			} else if (c == '=') {
				/* value contains = */
				return -EILSEQ;
			}
		}
	}

	/* qs ends with a name without a '=' (e.g., abc=def&ghi) */
	if ((state == QS_STATE_NAME) && (name < end))
		insert(nvl, name, end - name, NULL, 0);
	/* qs ends with an empty value (e.g., abc=def&ghi=) */
	if ((state == QS_STATE_VAL) && (val == end))
		insert(nvl, name, name_len, val, 0);
	/* qs ends with a value (e.g., abc=def&ghi=jkl) */
	if ((state == QS_STATE_VAL) && (val < end))
		insert(nvl, name, name_len, val, end - val);

	return 0;
}
