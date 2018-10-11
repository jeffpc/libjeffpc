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

#include <jeffpc/nvl.h>
#include <jeffpc/hexdump.h>
#include <jeffpc/sexpr.h>
#include <jeffpc/io.h>

#include "test-file.c"

static inline struct nvlist *alloc(void)
{
	struct nvlist *nvl;

	nvl = nvl_alloc();
	if (!nvl)
		fail("nvl_alloc() failed");

	return nvl;
}

static inline void set_bool(struct nvlist *nvl, const char *key, bool b,
			    int expected_ret)
{
	const char *bstr = b ? "true" : "false";
	int ret;

	ret = nvl_set_bool(nvl, key, b);

	if (ret == expected_ret)
		return;

	if (ret && !expected_ret)
		fail("nvl_set_bool(..., '%s', %s) returned %s, expected success",
		     key, bstr, xstrerror(ret));

	if (ret && expected_ret)
		fail("nvl_set_bool(..., '%s', %s) returned %s, expected %s", key,
		     bstr, xstrerror(ret), xstrerror(expected_ret));

	if (!ret && expected_ret)
		fail("nvl_set_bool(..., '%s', %s) succeded, expected %s", key,
		     bstr, xstrerror(expected_ret));
}

static inline void set_int(struct nvlist *nvl, const char *key, uint64_t i,
			   int expected_ret)
{
	int ret;

	ret = nvl_set_int(nvl, key, i);

	if (ret == expected_ret)
		return;

	if (ret && !expected_ret)
		fail("nvl_set_int(..., '%s', %"PRIu64") returned %s, "
		     "expected success", key, i, xstrerror(ret));

	if (ret && expected_ret)
		fail("nvl_set_int(..., '%s', %"PRIu64") returned %s, "
		     "expected %s", key, i, xstrerror(ret),
		     xstrerror(expected_ret));

	if (!ret && expected_ret)
		fail("nvl_set_int(..., '%s', %"PRIu64") succeded, "
		     "expected %s", key, i, xstrerror(expected_ret));
}

static inline void set_null(struct nvlist *nvl, const char *key,
			    int expected_ret)
{
	int ret;

	ret = nvl_set_null(nvl, key);

	if (ret == expected_ret)
		return;

	if (ret && !expected_ret)
		fail("nvl_set_null(..., '%s') returned %s, expected success",
		     key, xstrerror(ret));

	if (ret && expected_ret)
		fail("nvl_set_null(..., '%s') returned %s, expected %s", key,
		     xstrerror(ret), xstrerror(expected_ret));

	if (!ret && expected_ret)
		fail("nvl_set_null(..., '%s') succeded, expected %s", key,
		     xstrerror(expected_ret));
}

static inline void set_str(struct nvlist *nvl, const char *key, struct str *str,
			   int expected_ret)
{
	int ret;

	ret = nvl_set_str(nvl, key, str);

	if (ret == expected_ret)
		return;

	if (ret && !expected_ret)
		fail("nvl_set_str(..., '%s', ...) returned %s, expected success",
		     key, xstrerror(ret));

	if (ret && expected_ret)
		fail("nvl_set_str(..., '%s', ...) returned %s, expected %s", key,
		     xstrerror(ret), xstrerror(expected_ret));

	if (!ret && expected_ret)
		fail("nvl_set_str(..., '%s', ...) succeded, expected %s", key,
		     xstrerror(expected_ret));
}

static inline void unset(struct nvlist *nvl, const char *key, int expected_ret)
{
	int ret;

	ret = nvl_unset(nvl, key);

	if (ret == expected_ret)
		return;

	if (ret && !expected_ret)
		fail("nvl_unset(..., '%s') returned %s, expected success", key,
		     xstrerror(ret));

	if (ret && expected_ret)
		fail("nvl_unset(..., '%s') returned %s, expected %s", key,
		     xstrerror(ret), xstrerror(expected_ret));

	if (!ret && expected_ret)
		fail("nvl_unset(..., '%s') succeded, expected %s", key,
		     xstrerror(expected_ret));
}

static inline void dumpbuf(struct buffer *buf, bool hex)
{
	const size_t len = buffer_used(buf);
	char tmp[len * 2 + 1];

	if (hex) {
		hexdumpz(tmp, buffer_data(buf), len, false);
		fprintf(stderr, "%s", tmp);
	} else {
		fprintf(stderr, "%*.*s", len, len,
			(const char *) buffer_data(buf));
	}
}

static void cmp_buffers(const char *name, bool hex,
			struct buffer *exp, struct buffer *got)
{
	fprintf(stderr, "  %s expected: ", name);
	dumpbuf(exp, hex);
	fprintf(stderr, "\n");
	fprintf(stderr, "  %s got:      ", name);
	dumpbuf(got, hex);
	fprintf(stderr, "\n");

	if (buffer_used(got) != buffer_used(exp))
		fail("%s packing failed: length mismatch "
		     "(got %zu, expected %zu)", name, buffer_used(got),
		     buffer_used(exp));

	if (memcmp(buffer_data(got), buffer_data(exp), buffer_used(got)))
		fail("%s packing failed: content mismatch", name);
}

static inline void check_packing_fmt(const char *name, bool hex,
				     struct nvlist *nvl,
				     struct buffer *expected,
				     enum val_format fmt)
{
	const size_t rawsize = buffer_used(expected);
	uint8_t *raw = alloca(rawsize);
	struct buffer *buf;
	struct buffer tmp;
	ssize_t ret;

	fprintf(stderr, "nvl_pack:\n");

	buf = nvl_pack(nvl, fmt);
	cmp_buffers(name, hex, expected, buf);
	buffer_free(buf);

	fprintf(stderr, "nvl_pack_into:\n");

	ret = nvl_pack_into(nvl, raw, rawsize, fmt);
	buffer_init_static(&tmp, raw, (ret > 0) ? ret : 0, false);
	cmp_buffers(name, hex, expected, &tmp);
}

static inline void check_packing(struct nvlist *nvl,
				 struct buffer *expected_json,
				 struct buffer *expected_cbor)
{
	check_packing_fmt("JSON", false, nvl, expected_json, VF_JSON);
	check_packing_fmt("CBOR", true, nvl, expected_cbor, VF_CBOR);
}

static void onefile(struct val *prog,
		    struct buffer *expected_json,
		    struct buffer *expected_cbor)
{
	struct val *tmp;
	struct val *cur;
	struct nvlist *nvl;

	val_dump_file(stderr, prog, 0);

	nvl = alloc();

	sexpr_for_each(cur, tmp, prog) {
		struct val *op[3];
		struct sym *opname;
		int nargs;
		int ret;

		ret = sexpr_list_to_array(cur, op, ARRAY_LEN(op));
		if (ret < 0)
			fail("failed to split up op list");
		if (ret < 1)
			fail("missing op name");

		opname = val_cast_to_sym(op[0]);
		nargs = ret - 1;

		if (!strcmp(sym_cstr(opname), "CLEAR")) {
			/* it is easier to just reallocate it */
			if (nargs != 0)
				fail("CLEAR requires 0 args, got %d", nargs);

			nvl_putref(nvl);

			nvl = alloc();
		} else if (!strcmp(sym_cstr(opname), "SET-VAL")) {
			if (nargs != 2)
				fail("SET-VAL requires 2 args, got %d", nargs);

			fail("SET-VAL '%s': not yet implemented");
		} else if (!strcmp(sym_cstr(opname), "SET")) {
			const char *var;

			if (nargs != 2)
				fail("SET requires 2 args, got %d", nargs);

			var = str_cstr(val_cast_to_str(op[1]));

			switch (op[2]->type) {
				case VT_CHAR:
				case VT_CONS:
				case VT_SYM:
					fail("nvlists don't support chars/cons/sym");
					break;
				case VT_BLOB:
				case VT_ARRAY:
				case VT_NVL:
					/*
					 * We couldn't have possibly read
					 * this from the input sexpr!
					 */
					fail("sexprs don't support "
					     "blobs/arrays/nvlists");
					break;
				case VT_BOOL:
					set_bool(nvl, var, op[2]->b, 0);
					break;
				case VT_INT:
					set_int(nvl, var, op[2]->i, 0);
					break;
				case VT_NULL:
					set_null(nvl, var, 0);
					break;
				case VT_STR:
					set_str(nvl, var, val_getref_str(op[2]), 0);
					break;
			}
		} else if (!strcmp(sym_cstr(opname), "UNSET")) {
			const char *var;

			if (nargs != 2)
				fail("UNSET requires 2 args, got %d", nargs);

			var = str_cstr(val_cast_to_str(op[1]));

			ASSERT3U(op[2]->type, ==, VT_BOOL);

			unset(nvl, var, op[2]->b ? 0 : -EINVAL);
		} else {
			fail("unknown op '%s'", sym_cstr(opname));
		}
	}

	check_packing(nvl, expected_json, expected_cbor);

	nvl_putref(nvl);
}

static void get_expected_output(const char *fname, const char *ext,
				struct buffer *buf)
{
	char expfname[FILENAME_MAX];
	size_t len;
	char *tmp;

	VERIFY3U(strlen(ext), <=, strlen("lisp"));

	/* replace .lisp with .ext */
	strcpy(expfname, fname);
	strcpy(expfname + strlen(expfname) - 4, ext);

	tmp = read_file_len(expfname, &len);
	ASSERT(!IS_ERR(tmp));

	buffer_init_static(buf, tmp, len, false);
}

static void test(const char *fname)
{
	struct buffer expected_json;
	struct buffer expected_cbor;
	struct val *lv;
	char *in;

	in = read_file(fname);
	if (IS_ERR(in))
		fail("failed to read input (%s)", xstrerror(PTR_ERR(in)));

	lv = sexpr_parse(in, strlen(in));
	if (IS_ERR(lv))
		fail("failed to parse input: %s", xstrerror(PTR_ERR(lv)));

	free(in);

	get_expected_output(fname, "json", &expected_json);
	get_expected_output(fname, "cbor", &expected_cbor);

	onefile(lv, &expected_json, &expected_cbor);

	free((void *) buffer_data(&expected_json));
	free((void *) buffer_data(&expected_cbor));
}
