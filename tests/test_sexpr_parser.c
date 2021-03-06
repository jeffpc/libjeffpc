/*
 * Copyright (c) 2015-2019 Josef 'Jeff' Sipek <jeffpc@josefsipek.net>
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

#include <jeffpc/error.h>
#include <jeffpc/sexpr.h>
#include <jeffpc/val.h>
#include <jeffpc/io.h>

#include "test.c"

static void trim(char *ptr, size_t *len)
{
	if (!*len)
		return;

	if (ptr[*len - 1] == '\n') {
		ptr[*len - 1] = '\0';
		(*len)--;
	}
}

static void check_file(struct val *got, const char *exp, bool raw)
{
	const char *pfx = raw ? "raw   " : "pretty";
	struct str *dumped;

	fprintf(stderr, "%s exp: %s\n", pfx, exp);

	dumped = sexpr_dump(got, raw);
	if (IS_ERR(dumped))
		fail("failed to dump val: %s", xstrerror(PTR_ERR(dumped)));

	fprintf(stderr, "%s got: %s\n", pfx, str_cstr(dumped));

	if (strcmp(str_cstr(dumped), exp))
		fail("mismatch!");

	str_putref(dumped);
}

void test(const char *ifname, void *in, size_t ilen, const char *iext,
	  const char *ofname, void *out, size_t olen, const char *oext)
{
	struct val *lv;

	trim(in, &ilen);
	trim(out, &olen);

	fprintf(stderr, "input     : %s\n", (char *) in);

	lv = sexpr_parse(in, ilen);
	if (IS_ERR(lv))
		fail("failed to parse input: %s", xstrerror(PTR_ERR(lv)));

	check_file(lv, out, !strcmp(oext, "raw"));

	val_putref(lv);
}
