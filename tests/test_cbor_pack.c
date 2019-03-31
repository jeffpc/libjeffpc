/*
 * Copyright (c) 2017-2019 Josef 'Jeff' Sipek <jeffpc@josefsipek.net>
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
#include <jeffpc/mem.h>

#include "test.c"

static inline void dumpbuf(struct buffer *buf)
{
	const size_t len = buffer_size(buf);
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

	if (buffer_size(got) != buffer_size(exp))
		fail("cbor packing failed: length mismatch "
		     "(got %zu, expected %zu)", buffer_size(got),
		     buffer_size(exp));

	if (memcmp(buffer_data(got), buffer_data(exp), buffer_size(got)))
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

static struct val *convert_input(struct val *input)
{
	struct val **arr;
	size_t len;
	size_t i;
	int ret;

	if (input->type != VT_CONS)
		return input;

	/*
	 * We are dealing with either a list or an assoc, we need to figure
	 * out which it is...
	 * FIXME
	 */

	if (0) {
		/* it is an assoc; turn it into a VT_NVL */
		fail("not yet implemented");
	} else {
		/* it is a list; turn it into a VT_ARRAY */
		len = sexpr_length(val_getref(input));
		if (len < 0)
			fail("failed to get length of array");

		arr = mem_reallocarray(NULL, len, sizeof(struct val *));
		if (!arr)
			fail("failed to allocate val array");

		ret = sexpr_list_to_array(input, arr, len);
		if (ret < 0)
			fail("failed to construct an array");

		for (i = 0; i < len; i++)
			arr[i] = convert_input(arr[i]);

		val_putref(input);

		return VAL_ALLOC_ARRAY(arr, len);
	}
}

static void onefile(struct val *input, struct buffer *expected)
{
	struct buffer *got;

	fprintf(stderr, "input: ");
	val_dump_file(stderr, input, 0);

	got = buffer_alloc(1000);
	if (IS_ERR(got))
		fail("failed to allocate output buffer");

	/* possibly an VT_ARRAY or VT_NVL in sexpr list form */
	if (input->type == VT_CONS)
		input = convert_input(input);

	fprintf(stderr, "modified input: ");
	val_dump_file(stderr, input, 0);

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
		case VT_ARRAY:
			TEST_ONE(cbor_pack_array_vals(got, input->array.vals,
						      input->array.nelem),
				 got, expected, input);
			break;
		case VT_NVL:
			TEST_ONE(cbor_pack_map_val(got, input), got,
				 expected, input);
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

void test(const char *ifname, const void *in, size_t ilen, const char *iext,
	  const char *ofname, const void *out, size_t olen, const char *oext)
{
	struct buffer expected;
	struct val *lv;

	lv = sexpr_parse(in, ilen);
	if (IS_ERR(lv))
		fail("failed to parse input: %s", xstrerror(PTR_ERR(lv)));

	buffer_init_static(&expected, out, olen, false);

	onefile(lv, &expected);
}
