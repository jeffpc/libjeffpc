/*
 * Copyright (c) 2017-2018 Josef 'Jeff' Sipek <jeffpc@josefsipek.net>
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

#include <jeffpc/hexdump.h>
#include <jeffpc/sexpr.h>
#include <jeffpc/cbor.h>
#include <jeffpc/io.h>

#include "test-file.c"

static inline void dumpbuf(struct buffer *buf)
{
	const size_t len = buffer_used(buf);
	char tmp[len * 2 + 1];

	hexdumpz(tmp, buffer_data(buf), len, false);
	fprintf(stderr, "%s", tmp);
}

static void cmp_buffers(struct buffer *exp, struct buffer *got)
{
	fprintf(stderr, "  expected: ");
	dumpbuf(exp);
	fprintf(stderr, "\n");
	fprintf(stderr, "  got:      ");
	dumpbuf(got);
	fprintf(stderr, "\n");

	if (buffer_used(got) != buffer_used(exp))
		fail("cbor packing failed: length mismatch "
		     "(got %zu, expected %zu)", buffer_used(got),
		     buffer_used(exp));

	if (memcmp(buffer_data(got), buffer_data(exp), buffer_used(got)))
		fail("cbor packing failed: content mismatch");
}

#define TEST_ONE(pack, got, exp, input)					\
	do {								\
		int ret;						\
									\
		buffer_truncate((got), 0);				\
									\
		fprintf(stderr, "pack via: %s\n", #pack);		\
		ret = pack;						\
		if (ret)						\
			fail("failed to pack value directly: %s",	\
			     xstrerror(ret));				\
									\
		cmp_buffers((exp), (got));				\
									\
		buffer_truncate((got), 0);				\
									\
		fprintf(stderr, "pack via: %s\n",			\
		        "cbor_pack_val(got, input)");			\
		ret = cbor_pack_val((got), (input));			\
		if (ret)						\
			fail("failed to pack value indirectly: %s",	\
			     xstrerror(ret));				\
									\
		cmp_buffers((exp), (got));				\
	} while (0)

static void onefile(struct val *input, struct buffer *expected)
{
	struct buffer *got;

	fprintf(stderr, "input: ");
	val_dump_file(stderr, input, 0);

	got = buffer_alloc(1000);
	if (IS_ERR(got))
		fail("failed to allocate output buffer");

	switch (input->type) {
		case VT_NULL:
			TEST_ONE(cbor_pack_null(got), got, expected, input);
			break;
		case VT_INT:
			TEST_ONE(cbor_pack_uint(got, input->i), got, expected,
				 input);
			break;
		case VT_BOOL:
			TEST_ONE(cbor_pack_bool(got, input->b), got, expected,
				 input);
			break;
		case VT_STR:
			TEST_ONE(cbor_pack_str(got, val_cast_to_str(input)),
				 got, expected, input);
			break;
		case VT_SYM:
		case VT_CONS:
		case VT_CHAR:
		case VT_BLOB:
			fail("Unsupported val type");
			break;
	}

	val_putref(input);
}

static void get_expected_output(const char *fname, struct buffer *buf)
{
	char expfname[FILENAME_MAX];
	size_t len;
	char *tmp;

	VERIFY3U(strlen("cbor"), <=, strlen("lisp"));

	/* replace .lisp with .ext */
	strcpy(expfname, fname);
	strcpy(expfname + strlen(expfname) - 4, "cbor");

	tmp = read_file_len(expfname, &len);
	ASSERT(!IS_ERR(tmp));

	buffer_init_const(buf, tmp, len);
}

static void test(const char *fname)
{
	struct buffer expected;
	struct val *lv;
	char *in;

	in = read_file(fname);
	if (IS_ERR(in))
		fail("failed to read input (%s)", xstrerror(PTR_ERR(in)));

	lv = sexpr_parse(in, strlen(in));
	if (IS_ERR(lv))
		fail("failed to parse input: %s", xstrerror(PTR_ERR(lv)));

	free(in);

	get_expected_output(fname, &expected);

	onefile(lv, &expected);

	free((void *) buffer_data(&expected));
}
