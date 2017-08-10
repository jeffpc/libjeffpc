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
#include <jeffpc/str.h>
#include <jeffpc/val.h>
#include <jeffpc/io.h>

static int onefile(char *ibuf, size_t len)
{
	struct val *lv;
	int ret;

	lv = sexpr_parse(ibuf, len);
	if (IS_ERR(lv))
		return PTR_ERR(lv);

	ret = sexpr_dump_file(stdout, lv, false);

	if (!ret)
		printf("\n");

	val_putref(lv);

	return ret;
}

int main(int argc, char **argv)
{
	char *in;
	int i;
	int result;

	result = 0;

	for (i = 1; i < argc; i++) {
		int ret;

		fprintf(stderr, "Checking %s...", argv[i]);

		in = read_file(argv[i]);
		if (IS_ERR(in)) {
			fprintf(stderr, "failed to read input (%s)\n",
				xstrerror(PTR_ERR(in)));
			result = 1;
			continue;
		}

		ret = onefile(in, strlen(in));
		if (ret) {
			fprintf(stderr, "failed to parse (%s)\n",
				xstrerror(ret));
			result = 1;
		} else {
			fprintf(stderr, "ok.\n");
		}

		free(in);
	}

	return result;
}
