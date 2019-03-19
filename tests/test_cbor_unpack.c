/*
 * Copyright (c) 2018-2019 Josef 'Jeff' Sipek <jeffpc@josefsipek.net>
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
#include <jeffpc/buffer.h>
#include <jeffpc/val.h>
#include <jeffpc/sexpr.h>
#include <jeffpc/io.h>
#include <jeffpc/cbor.h>

#include "test.c"

static inline void dumpbuf(struct buffer *buf)
{
	const size_t len = buffer_size(buf);
	char tmp[len * 2 + 1];

	hexdumpz(tmp, buffer_data(buf), len, false);
	fprintf(stderr, "%s", tmp);
}

#define RUN_ONE_SIMPLE(fxn, exp_ret, alloc, in, exp)				\
	do {								\
		struct buffer tmp;					\
		struct val *wrap;					\
		int ret;						\
									\
		buffer_init_static(&tmp, buffer_data(in),		\
				   buffer_size(in), false);		\
									\
		fprintf(stderr, "unpack via %s (should %s)...",		\
			#fxn, (exp_ret) ? "fail" : "succeed");		\
									\
		ret = fxn;						\
									\
		check_rets((exp_ret), ret,				\
			   "failed to unpack value directly");		\
									\
		if (ret) {						\
			/* failed, so there is no value to compare */	\
			fprintf(stderr, "ok.\n");			\
			break;						\
		}							\
									\
		wrap = alloc;						\
		if (!sexpr_equal(wrap, val_getref(exp)))		\
			fail("not equal");				\
									\
		/* TODO: unpack via cbor_unpack_val & compare value */	\
		fprintf(stderr, "ok.\n");				\
	} while (0)

#define VINT(u)		VAL_ALLOC_INT(u)
#define VSTR(s)		VAL_ALLOC_STR_STATIC(s)
#define VBOOL(b)	VAL_ALLOC_BOOL(b)
#define VNULL()		VAL_ALLOC_NULL()

#define VSTRCAST(s)	str_getref_val(s)

static void check_null(struct buffer *in, struct val *exp)
{
	uint64_t nelem, npairs;
	bool end_required;
	struct str *str;
	uint64_t u;
	int64_t i;
	size_t ss;
	char *s;
	bool b;

	RUN_ONE_SIMPLE(cbor_unpack_uint(&tmp, &u), -EILSEQ, NULL, in, exp);
	RUN_ONE_SIMPLE(cbor_unpack_nint(&tmp, &u), -EILSEQ, NULL, in, exp);
	RUN_ONE_SIMPLE(cbor_unpack_int(&tmp, &i),  -EILSEQ, NULL, in, exp);
	RUN_ONE_SIMPLE(cbor_unpack_cstr_len(&tmp, &s, &ss),
		                            -EILSEQ, NULL, in, exp);
	RUN_ONE_SIMPLE(cbor_unpack_str(&tmp, &str),-EILSEQ, NULL, in, exp);
	RUN_ONE_SIMPLE(cbor_unpack_bool(&tmp, &b), -EILSEQ, NULL, in, exp);
	RUN_ONE_SIMPLE(cbor_unpack_null(&tmp),     0,       VNULL(), in, exp);

	/* only need to check the starts & forced ends */
	RUN_ONE_SIMPLE(cbor_unpack_array_start(in, &nelem, &end_required),
					    -EILSEQ, NULL, in, exp);
	RUN_ONE_SIMPLE(cbor_unpack_array_end(in, true),
					    -EILSEQ, NULL, in, exp);

	/* only need to check the starts & forced ends */
	RUN_ONE_SIMPLE(cbor_unpack_map_start(in, &npairs, &end_required),
					    -EILSEQ, NULL, in, exp);
	RUN_ONE_SIMPLE(cbor_unpack_map_end(in, true),
					    -EILSEQ, NULL, in, exp);
}

static void check_bool(struct buffer *in, struct val *exp)
{
	uint64_t nelem, npairs;
	bool end_required;
	struct str *str;
	uint64_t u;
	int64_t i;
	size_t ss;
	char *s;
	bool b;

	RUN_ONE_SIMPLE(cbor_unpack_uint(&tmp, &u), -EILSEQ, NULL, in, exp);
	RUN_ONE_SIMPLE(cbor_unpack_nint(&tmp, &u), -EILSEQ, NULL, in, exp);
	RUN_ONE_SIMPLE(cbor_unpack_int(&tmp, &i),  -EILSEQ, NULL, in, exp);
	RUN_ONE_SIMPLE(cbor_unpack_cstr_len(&tmp, &s, &ss),
		                            -EILSEQ, NULL, in, exp);
	RUN_ONE_SIMPLE(cbor_unpack_str(&tmp, &str),-EILSEQ, NULL, in, exp);
	RUN_ONE_SIMPLE(cbor_unpack_bool(&tmp, &b), 0,       VBOOL(b), in, exp);
	RUN_ONE_SIMPLE(cbor_unpack_null(&tmp),     -EILSEQ, NULL, in, exp);

	/* only need to check the starts & forced ends */
	RUN_ONE_SIMPLE(cbor_unpack_array_start(in, &nelem, &end_required),
					    -EILSEQ, NULL, in, exp);
	RUN_ONE_SIMPLE(cbor_unpack_array_end(in, true),
					    -EILSEQ, NULL, in, exp);

	/* only need to check the starts & forced ends */
	RUN_ONE_SIMPLE(cbor_unpack_map_start(in, &npairs, &end_required),
					    -EILSEQ, NULL, in, exp);
	RUN_ONE_SIMPLE(cbor_unpack_map_end(in, true),
					    -EILSEQ, NULL, in, exp);
}

static void check_int(struct buffer *in, struct val *exp)
{
	const int int_ret = (exp->i > INT64_MAX) ? -EOVERFLOW : 0;
	uint64_t nelem, npairs;
	bool end_required;
	struct str *str;
	uint64_t u;
	int64_t i;
	size_t ss;
	char *s;
	bool b;

	RUN_ONE_SIMPLE(cbor_unpack_uint(&tmp, &u), 0,       VINT(u), in, exp);
	RUN_ONE_SIMPLE(cbor_unpack_nint(&tmp, &u), -EILSEQ, NULL, in, exp);
	RUN_ONE_SIMPLE(cbor_unpack_int(&tmp, &i),  int_ret, VINT(i), in, exp);
	RUN_ONE_SIMPLE(cbor_unpack_cstr_len(&tmp, &s, &ss),
		                            -EILSEQ, NULL, in, exp);
	RUN_ONE_SIMPLE(cbor_unpack_str(&tmp, &str),-EILSEQ, NULL, in, exp);
	RUN_ONE_SIMPLE(cbor_unpack_bool(&tmp, &b), -EILSEQ, NULL, in, exp);
	RUN_ONE_SIMPLE(cbor_unpack_null(&tmp),     -EILSEQ, NULL, in, exp);

	/* only need to check the starts & forced ends */
	RUN_ONE_SIMPLE(cbor_unpack_array_start(in, &nelem, &end_required),
					    -EILSEQ, NULL, in, exp);
	RUN_ONE_SIMPLE(cbor_unpack_array_end(in, true),
					    -EILSEQ, NULL, in, exp);

	/* only need to check the starts & forced ends */
	RUN_ONE_SIMPLE(cbor_unpack_map_start(in, &npairs, &end_required),
					    -EILSEQ, NULL, in, exp);
	RUN_ONE_SIMPLE(cbor_unpack_map_end(in, true),
					    -EILSEQ, NULL, in, exp);
}

static void check_str(struct buffer *in, struct val *exp)
{
	uint64_t nelem, npairs;
	bool end_required;
	struct str *str;
	uint64_t u;
	int64_t i;
	size_t ss;
	char *s;
	bool b;

	RUN_ONE_SIMPLE(cbor_unpack_uint(&tmp, &u), -EILSEQ, NULL, in, exp);
	RUN_ONE_SIMPLE(cbor_unpack_nint(&tmp, &u), -EILSEQ, NULL, in, exp);
	RUN_ONE_SIMPLE(cbor_unpack_int(&tmp, &i),  -EILSEQ, NULL, in, exp);
	RUN_ONE_SIMPLE(cbor_unpack_cstr_len(&tmp, &s, &ss),
		                            0,       VSTR(s), in, exp);
	RUN_ONE_SIMPLE(cbor_unpack_str(&tmp, &str),0,       VSTRCAST(str), in, exp);
	RUN_ONE_SIMPLE(cbor_unpack_bool(&tmp, &b), -EILSEQ, NULL, in, exp);
	RUN_ONE_SIMPLE(cbor_unpack_null(&tmp),     -EILSEQ, NULL, in, exp);

	/* only need to check the starts & forced ends */
	RUN_ONE_SIMPLE(cbor_unpack_array_start(in, &nelem, &end_required),
					    -EILSEQ, NULL, in, exp);
	RUN_ONE_SIMPLE(cbor_unpack_array_end(in, true),
					    -EILSEQ, NULL, in, exp);

	/* only need to check the starts & forced ends */
	RUN_ONE_SIMPLE(cbor_unpack_map_start(in, &npairs, &end_required),
					    -EILSEQ, NULL, in, exp);
	RUN_ONE_SIMPLE(cbor_unpack_map_end(in, true),
					    -EILSEQ, NULL, in, exp);
}

static void check_arr(struct buffer *in, struct val *exp)
{
	uint64_t npairs;
	bool end_required;
	struct str *str;
	uint64_t u;
	int64_t i;
	size_t ss;
	char *s;
	bool b;

	RUN_ONE_SIMPLE(cbor_unpack_uint(&tmp, &u), -EILSEQ, NULL, in, exp);
	RUN_ONE_SIMPLE(cbor_unpack_nint(&tmp, &u), -EILSEQ, NULL, in, exp);
	RUN_ONE_SIMPLE(cbor_unpack_int(&tmp, &i),  -EILSEQ, NULL, in, exp);
	RUN_ONE_SIMPLE(cbor_unpack_cstr_len(&tmp, &s, &ss),
		                            -EILSEQ, NULL, in, exp);
	RUN_ONE_SIMPLE(cbor_unpack_str(&tmp, &str),-EILSEQ, NULL, in, exp);
	RUN_ONE_SIMPLE(cbor_unpack_bool(&tmp, &b), -EILSEQ, NULL, in, exp);
	RUN_ONE_SIMPLE(cbor_unpack_null(&tmp),     -EILSEQ, NULL, in, exp);

#if 0
	/* FIXME: RUN_ONE_SIMPLE resets the buffer state */
	RUN_ONE_SIMPLE(cbor_unpack_array_start(in, &npairs, &end_required),
					    0,       X, in, exp);
	/* TODO: unpack vals in a loop */
	RUN_ONE_SIMPLE(cbor_unpack_array_end(in, end_required),
					    0,       X, in, exp);
#endif

	/* only need to check the starts & forced ends */
	RUN_ONE_SIMPLE(cbor_unpack_map_start(in, &npairs, &end_required),
					    -EILSEQ, NULL, in, exp);
	RUN_ONE_SIMPLE(cbor_unpack_map_end(in, true),
					    -EILSEQ, NULL, in, exp);
}

static void check_nvl(struct buffer *in, struct val *exp)
{
	uint64_t nelem;
	bool end_required;
	struct str *str;
	uint64_t u;
	int64_t i;
	size_t ss;
	char *s;
	bool b;

	RUN_ONE_SIMPLE(cbor_unpack_uint(&tmp, &u), -EILSEQ, NULL, in, exp);
	RUN_ONE_SIMPLE(cbor_unpack_nint(&tmp, &u), -EILSEQ, NULL, in, exp);
	RUN_ONE_SIMPLE(cbor_unpack_int(&tmp, &i),  -EILSEQ, NULL, in, exp);
	RUN_ONE_SIMPLE(cbor_unpack_cstr_len(&tmp, &s, &ss),
		                            -EILSEQ, NULL, in, exp);
	RUN_ONE_SIMPLE(cbor_unpack_str(&tmp, &str),-EILSEQ, NULL, in, exp);
	RUN_ONE_SIMPLE(cbor_unpack_bool(&tmp, &b), -EILSEQ, NULL, in, exp);
	RUN_ONE_SIMPLE(cbor_unpack_null(&tmp),     -EILSEQ, NULL, in, exp);

	/* only need to check the starts & forced ends */
	RUN_ONE_SIMPLE(cbor_unpack_array_start(in, &nelem, &end_required),
					    -EILSEQ, NULL, in, exp);
	RUN_ONE_SIMPLE(cbor_unpack_array_end(in, true),
					    -EILSEQ, NULL, in, exp);

#if 0
	/* FIXME: RUN_ONE_SIMPLE resets the buffer state */
	RUN_ONE_SIMPLE(cbor_unpack_map_start(in, &npairs, &end_required),
					    0,       X, in, exp);
	/* TODO: unpack vals in a loop */
	RUN_ONE_SIMPLE(cbor_unpack_map_end(in, end_required),
					    0,       X, in, exp);
#endif
}

static void check_cons(struct buffer *in, struct val *exp)
{
	enum val_type type;
	int ret;

	if (!sexpr_is_null(exp))
		fail("non-empty cons is not supported");

	ret = cbor_peek_type(in, &type);
	check_rets(0, ret, "failed to peek into input buffer");

	val_putref(exp);

	switch (type) {
		case VT_ARRAY:
			check_arr(in, VAL_ALLOC_ARRAY_STATIC(NULL, 0));
			break;
		case VT_NVL:
			check_nvl(in, VAL_ALLOC_NVL());
			break;
		case VT_NULL:
		case VT_INT:
		case VT_STR:
		case VT_SYM:
		case VT_BOOL:
		case VT_CONS:
		case VT_CHAR:
		case VT_BLOB:
			fail("peeked type doesn't match empty cons");
	}
}

static void onefile(struct buffer *in, struct val *expected)
{
	struct val *orig_expected = expected;

	fprintf(stderr, "input:  ");
	dumpbuf(in);
	fprintf(stderr, "\n");
	fprintf(stderr, "expect:\n");
	val_dump(expected, 1);

	expected = sexpr_compact(expected);
	ASSERT(!IS_ERR(expected));

	if (expected != orig_expected) {
		fprintf(stderr, "expect (compacted):\n");
		val_dump(expected, 1);
	}

	switch (expected->type) {
		case VT_NULL:
			check_null(in, expected);
			break;
		case VT_INT:
			check_int(in, expected);
			break;
		case VT_BOOL:
			check_bool(in, expected);
			break;
		case VT_STR:
			check_str(in, expected);
			break;
		case VT_ARRAY:
			check_arr(in, expected);
			break;
		case VT_NVL:
			check_nvl(in, expected);
			break;
		case VT_CONS:
			/*
			 * Empty nvlists and empty arrays look the same when
			 * serialized as an sexpr.  Therefore, we need to
			 * peek at the input buffer to know what to expect.
			 */
			check_cons(in, expected);
			break;
		case VT_SYM:
		case VT_CHAR:
		case VT_BLOB:
			fail("Unsupported val type");
			break;
	}

	val_putref(expected);
}

static struct val *get_expected_output(const char *fname)
{
	char expfname[FILENAME_MAX];
	struct val *lv;
	size_t len;
	char *tmp;

	VERIFY3U(strlen("lisp"), <=, strlen("cbor"));

	/* replace .cbor with .lisp */
	strcpy(expfname, fname);
	strcpy(expfname + strlen(expfname) - 4, "lisp");

	tmp = read_file_len(expfname, &len);
	ASSERT(!IS_ERR(tmp));

	lv = sexpr_parse(tmp, len);
	if (IS_ERR(lv))
		fail("failed to parse input: %s", xstrerror(PTR_ERR(lv)));

	free(tmp);

	return lv;
}

static void test(const char *fname)
{
	struct buffer input;
	size_t len;
	char *in;

	in = read_file_len(fname, &len);
	if (IS_ERR(in))
		fail("failed to read input (%s)", xstrerror(PTR_ERR(in)));

	buffer_init_static(&input, in, len, false);

	onefile(&input, get_expected_output(fname));

	free((void *) buffer_data(&input));
}
