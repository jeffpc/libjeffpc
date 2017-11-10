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

#include <stdlib.h>

#include <jeffpc/error.h>
#include <jeffpc/sexpr.h>
#include <jeffpc/val.h>
#include <jeffpc/io.h>

#include "test-file.c"

static void trim(char *ptr)
{
	size_t len = strlen(ptr);

	if (!len)
		return;

	if (ptr[len - 1] == '\n')
		ptr[len - 1] = '\0';
}

static void check_file(struct val *got, char *fname, bool raw)
{
	const char *pfx = raw ? "raw   " : "pretty";
	struct str *dumped;
	char *exp;

	exp = read_file(fname);
	if (IS_ERR(exp))
		fail("failed to read expected file (%s): %s", fname,
		     xstrerror(PTR_ERR(exp)));

	trim(exp);

	fprintf(stderr, "%s exp: %s\n", pfx, exp);

	dumped = sexpr_dump(got, raw);
	if (IS_ERR(dumped))
		fail("failed to dump val: %s", xstrerror(PTR_ERR(dumped)));

	fprintf(stderr, "%s got: %s\n", pfx, str_cstr(dumped));

	if (strcmp(str_cstr(dumped), exp))
		fail("mismatch!");

	free(exp);

	str_putref(dumped);
}

static void test(const char *fname)
{
	char expfname[FILENAME_MAX];
	struct val *lv;
	char *in;

	in = read_file(fname);
	if (IS_ERR(in))
		fail("failed to read input (%s)", xstrerror(PTR_ERR(in)));

	trim(in);

	fprintf(stderr, "input     : %s\n", in);

	lv = sexpr_parse(in, strlen(in));
	if (IS_ERR(lv))
		fail("failed to parse input: %s", xstrerror(PTR_ERR(lv)));

	free(in);

	/* replace .lisp with .raw & check it*/
	strcpy(expfname, fname);
	strcpy(expfname + strlen(expfname) - 4, "raw");
	check_file(lv, expfname, true);

	/* replace .lisp with .txt & check it*/
	strcpy(expfname, fname);
	strcpy(expfname + strlen(expfname) - 4, "txt");
	check_file(lv, expfname, false);

	val_putref(lv);
}
