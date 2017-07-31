/*
 * Copyright (c) 2010-2017 Josef 'Jeff' Sipek <jeffpc@josefsipek.net>
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
#include <stdio.h>
#include <string.h>

#include <jeffpc/urldecode.h>
#include <jeffpc/error.h>

/*
 * CGI 1.1 (RFC 3875) defines the QUERY_STRING variable to contain an
 * URL-ecoded string.  The specific flavor of encoding is based on the rules
 * in RFC 2396 (obsoleted by RFC 3986) and the HTML 4.01 Specification
 * (which defines the application/x-www-form-urlencoded MIME type) which
 * references RFC 1738 (which has been updated by 3986).
 *
 * The HTML 4.01 spec describes the algorithm for encoding a form:
 *
 *  1. control names and values are escaped
 *  2. spaces characters are replaced by '+'
 *  3. reserved characters are escaped according to RFC 1738 section 2.2
 *  4. each name is separated from its value with a '='
 *  5. each name/value pair is separated with a '&'
 *
 * Additionally, the SCGI RFC tells us that the HTML forms consider '+',
 * '&', and '=' as reserved.
 *
 * Note that we're not interesting in decoding the whole form blob but only
 * already separated out names and values.  Therefore, we should never
 * encounter '&' or '=' in the input.  (In other words, steps 4 and 5 have
 * already be undone.)  This leaves us with percent-encoding (per RFC 1738)
 * and the '+' to space translation.
 */

struct state {
	enum decode_state {
		DS_COPY,
		DS_ESC1,
		DS_ESC2,
	} state;

	const char *in;
	char *out;
	size_t len;
	size_t inoff;
	size_t outoff;
};

static inline void append_char(struct state *state, char c)
{
	VERIFY3S(state->state, ==, DS_COPY);
	VERIFY3U(c, !=, '+');
	VERIFY3U(c, !=, '%');

	state->out[state->outoff] = c;
	state->outoff++;
}

static inline int append_esc(struct state *state, char c)
{
	if ((c >= '0') && (c <= '9'))
		c -= '0';
	else if ((c >= 'a') && (c <= 'f'))
		c -= 'a' - 10;
	else if ((c >= 'A') && (c <= 'F'))
		c -= 'A' - 10;
	else
		return -EILSEQ;

	switch (state->state) {
		case DS_ESC1:
			state->out[state->outoff] = c << 4;
			state->state = DS_ESC2;
			break;
		case DS_ESC2:
			state->out[state->outoff] |= c;
			state->outoff++;
			state->state = DS_COPY;
			break;
		default:
			panic("illegal state when appending an escape");
	}

	return 0;
}

/*
 * Since urldecoding produces output that is <= the input length, the output
 * buffer is assumed to be the same size as the input.
 */
ssize_t urldecode(const char *in, size_t len, char *out)
{
	struct state state;
	int ret;

	if (!in || !out)
		return -EINVAL;

	if (!len)
		return 0;

	state.state = DS_COPY;
	state.in = in;
	state.out = out;
	state.len = len;
	state.inoff = 0;
	state.outoff = 0;

	while (state.inoff < state.len) {
		char c = state.in[state.inoff];

		switch (state.state) {
			case DS_COPY:
				/* copy the char unless it is special */
				switch (c) {
					case '%':
						state.state = DS_ESC1;
						break;
					case '+':
						append_char(&state, ' ');
						break;
					case '=':
					case '&':
						/*
						 * Even though we should
						 * never see these
						 * characters (as we're
						 * dealing with individual
						 * names/values that have
						 * been split up), we accept
						 * them as-is.
						 */
					default:
						append_char(&state, c);
						break;
				}

				ret = 0;
				break;
			case DS_ESC1:
			case DS_ESC2:
				/* first/second char of an escape sequence */
				ret = append_esc(&state, c);
				break;
		}

		if (ret)
			return ret;

		state.inoff++;
	}

	return (state.state == DS_COPY) ? state.outoff : -EILSEQ;
}

struct str *urldecode_str(const char *in, size_t len)
{
	struct str *str;
	ssize_t outlen;
	char out[len];

	outlen = urldecode(in, len, out);
	if (outlen < 0)
		return ERR_PTR(outlen);

	str = str_dup_len(out, outlen);
	if (!str)
		return ERR_PTR(-ENOMEM);

	return str;
}
