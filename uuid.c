/*
 * Copyright (c) 2015-2018 Josef 'Jeff' Sipek <jeffpc@josefsipek.net>
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

#include <jeffpc/uuid.h>
#include <jeffpc/hexdump.h>
#include <jeffpc/rand.h>

void xuuid_clear(struct xuuid *uuid)
{
	memset(uuid->raw, 0, sizeof(uuid->raw));
}

int xuuid_compare(const struct xuuid *u1, const struct xuuid *u2)
{
	return memcmp(u1->raw, u2->raw, sizeof(u1->raw));
}

/* generate a version 4 (random), variant 1 (big endian) UUID */
void xuuid_generate(struct xuuid *uuid)
{
	rand_buf(uuid->raw, sizeof(uuid->raw));

	/* version 4 */
	uuid->raw[6] &= 0x0f;
	uuid->raw[6] |= 0x40;

	/* variant 1 */
	uuid->raw[8] &= 0x3f;
	uuid->raw[8] |= 0x80;
}

static inline int parse_digit(char in)
{
	if ((in >= '0') && (in <= '9'))
		return in - '0';
	if ((in >= 'a') && (in <= 'f'))
		return in - 'a' + 10;
	if ((in >= 'A') && (in <= 'F'))
		return in - 'A' + 10;
	return -1;
}

static inline bool parse_part(uint8_t *out, const char *in, size_t len)
{
	size_t i;

	for (i = 0; i < len; i++) {
		int a = parse_digit(in[2 * i]);
		int b = parse_digit(in[2 * i + 1]);

		if ((a < 0) || (b < 0))
			return false;

		out[i] = (a << 4) | b;
	}

	return true;
}

bool xuuid_parse(struct xuuid *uuid, const char *in)
{
	return (in[8] == '-') && (in[13] == '-') &&
	       (in[18] == '-') && (in[23] == '-') &&
	       (in[36] == '\0') &&
	       parse_part(&uuid->raw[0], &in[0], 4) &&
	       parse_part(&uuid->raw[4], &in[9], 2) &&
	       parse_part(&uuid->raw[6], &in[14], 2) &&
	       parse_part(&uuid->raw[8], &in[19], 2) &&
	       parse_part(&uuid->raw[10], &in[24], 6);
}

void xuuid_unparse(const struct xuuid *uuid, char *out)
{
	/* hex dump the pieces */
	hexdump(&out[0], &uuid->raw[0], 4, false);
	hexdump(&out[9], &uuid->raw[4], 2, false);
	hexdump(&out[14], &uuid->raw[6], 2, false);
	hexdump(&out[19], &uuid->raw[8], 2, false);
	hexdump(&out[24], &uuid->raw[10], 6, false);

	/* fill in the dashes */
	out[8] = '-';
	out[13] = '-';
	out[18] = '-';
	out[23] = '-';

	/* trailing nul */
	out[36] = '\0';
}

bool_t xdr_xuuid(XDR *xdr, struct xuuid *obj)
{
	return xdr_opaque(xdr, (void *) obj->raw, sizeof(obj->raw));
}
