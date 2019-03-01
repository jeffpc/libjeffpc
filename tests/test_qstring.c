/*
 * Copyright (c) 2014-2019 Josef 'Jeff' Sipek <jeffpc@josefsipek.net>
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

#include <jeffpc/sexpr.h>
#include <jeffpc/qstring.h>
#include <jeffpc/io.h>

#include "test.c"

static int onefile(char *ibuf, size_t len, struct nvlist *out)
{
	struct nvlist *vars;
	int ret;

	vars = nvl_alloc();
	if (!vars)
		return -ENOMEM;

	ret = qstring_parse_len(vars, ibuf, len);
	if (!ret) {
		fprintf(stderr, "Got:\n");
		nvl_dump_file(stderr, vars);

		if (sexpr_equal(nvl_getref_val(out),
				 nvl_getref_val(vars)))
			fprintf(stderr, "ok.\n");
		else
			fail("mismatch!");
	}

	nvl_putref(vars);

	return ret;
}

static struct nvlist *get_expected(const char *infname)
{
	const size_t len = strlen(infname);
	char outfname[len + 3];
	struct val *outval;
	char *out;

	strcpy(outfname, infname); /* duplicate */
	outfname[len - 2] = '\0'; /* strip off 'qs' */
	strcat(outfname, "lisp"); /* append 'lisp' */

	out = read_file(outfname);
	ASSERT(!IS_ERR(out));

	outval = sexpr_parse(out, strlen(out));
	ASSERT(!IS_ERR(outval));

	outval = sexpr_compact(outval);
	ASSERT(!IS_ERR(outval));

	if (sexpr_is_null(outval)) {
		val_putref(outval);

		outval = val_alloc_nvl();
		ASSERT(!IS_ERR(outval));
	}

	return val_cast_to_nvl(outval);

}

static void test(const char *fname)
{
	struct nvlist *out;
	char *in;

	/* input */
	in = read_file(fname);
	ASSERT(!IS_ERR(in));

	/* output */
	out = get_expected(fname);

	fprintf(stderr, "Expected:\n");
	nvl_dump_file(stderr, out);

	if (onefile(in, strlen(in), out))
		fail("failed");

	nvl_putref(out);

	free(in);
}
