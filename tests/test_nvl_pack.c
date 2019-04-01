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

#include <jeffpc/mem.h>
#include <jeffpc/nvl.h>
#include <jeffpc/hexdump.h>
#include <jeffpc/sexpr.h>
#include <jeffpc/io.h>

#include "test.c"

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

	check_rets(expected_ret, ret, "nvl_set_bool(..., '%s', %s)", key,
		   bstr);
}

static inline void set_int(struct nvlist *nvl, const char *key, uint64_t i,
			   int expected_ret)
{
	int ret;

	ret = nvl_set_int(nvl, key, i);

	check_rets(expected_ret, ret, "nvl_set_int(..., '%s', %"PRIu64")", key,
		   i);
}

static inline void set_null(struct nvlist *nvl, const char *key,
			    int expected_ret)
{
	int ret;

	ret = nvl_set_null(nvl, key);

	check_rets(expected_ret, ret, "nvl_set_null(..., '%s')", key);
}

static inline void set_str(struct nvlist *nvl, const char *key, struct str *str,
			   int expected_ret)
{
	int ret;

	ret = nvl_set_str(nvl, key, str);

	check_rets(expected_ret, ret, "nvl_set_str(..., '%s', ...)", key);
}

static inline void unset(struct nvlist *nvl, const char *key, int expected_ret)
{
	int ret;

	ret = nvl_unset(nvl, key);

	check_rets(expected_ret, ret, "nvl_unset(..., '%s')", key);
}

static inline void dumpbuf(struct buffer *buf, bool hex)
{
	const size_t len = buffer_size(buf);
	char tmp[len * 2 + 1];

	if (hex) {
		hexdumpz(tmp, buffer_data(buf), len, false);
		fprintf(stderr, "%s", tmp);
	} else {
		VERIFY3U(len, <=, INT_MAX);
		fprintf(stderr, "%*.*s", (int) len, (int) len,
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

	if (buffer_size(got) != buffer_size(exp))
		fail("%s packing failed: length mismatch "
		     "(got %zu, expected %zu)", name, buffer_size(got),
		     buffer_size(exp));

	if (memcmp(buffer_data(got), buffer_data(exp), buffer_size(got)))
		fail("%s packing failed: content mismatch", name);
}

static inline void check_packing_fmt(const char *name, bool hex,
				     struct nvlist *nvl,
				     struct buffer *expected,
				     enum val_format fmt)
{
	const size_t rawsize = buffer_size(expected);
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

static inline void check_packing(struct nvlist *nvl, struct buffer *expected,
				 const char *fmt)
{
	enum val_format vfmt;
	bool binary;

	if (!strcmp(fmt, "cbor")) {
		vfmt = VF_CBOR;
		binary = true;
	} else if (!strcmp(fmt, "json")) {
		vfmt = VF_JSON;
		binary = false;
	} else {
		fail("unknown format '%s'", fmt);
	}

	check_packing_fmt(fmt, binary, nvl, expected, vfmt);
}

static void onefile(struct val *prog, struct buffer *expected,
		    const char *fmt)
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

	check_packing(nvl, expected, fmt);

	nvl_putref(nvl);
}

void test(const char *ifname, void *in, size_t ilen, const char *iext,
	  const char *ofname, void *out, size_t olen, const char *oext)
{
	struct buffer expected;
	struct val *lv;

	lv = sexpr_parse(in, ilen);
	if (IS_ERR(lv))
		fail("failed to parse input: %s", xstrerror(PTR_ERR(lv)));

	buffer_init_static(&expected, out, olen, false);

	onefile(lv, &expected, oext);
}
