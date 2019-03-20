/*
 * Copyright (c) 2019 Josef 'Jeff' Sipek <jeffpc@josefsipek.net>
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

#include <jeffpc/io.h>
#include <jeffpc/cbor.h>

static void dump(const char *fname)
{
	struct buffer input;
	struct val *val;
	size_t len;
	char *in;

	in = read_file_len(fname, &len);
	if (IS_ERR(in)) {
		fprintf(stderr, "Failed to read input file %s: %s\n", fname,
			xstrerror(PTR_ERR(in)));
		exit(1);
	}

	buffer_init_static(&input, in, len, false);

	val = cbor_unpack_val(&input);
	if (IS_ERR(val)) {
		fprintf(stderr, "Failed to unpack input file %s: %s\n", fname,
			xstrerror(PTR_ERR(val)));
		exit(2);
	}

	val_dump_file(stdout, val, 0);

	val_putref(val);
	free(in);
}

int main(int argc, char **argv)
{
	int i;

	for (i = 1; i < argc; i++)
		dump(argv[i]);

	return 0;
}
