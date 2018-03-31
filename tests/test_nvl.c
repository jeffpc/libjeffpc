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

#include "test.c"

static inline struct nvlist *alloc(void)
{
	struct nvlist *nvl;

	nvl = nvl_alloc();
	if (!nvl)
		fail("nvl_alloc() failed");

	return nvl;
}

static inline void merge(struct nvlist *dest, struct nvlist *src)
{
	int ret;

	ret = nvl_merge(dest, src);
	if (ret)
		fail("nvl_merge() failed: %s", xstrerror(ret));
}

static inline void set_bool(struct nvlist *nvl, const char *key, bool b)
{
	int ret;

	ret = nvl_set_bool(nvl, key, b);
	if (ret)
		fail("nvl_set_bool(..., '%s', %s) failed", key,
		     b ? "true" : "false");
}

static inline void set_int(struct nvlist *nvl, const char *key, uint64_t i)
{
	int ret;

	ret = nvl_set_int(nvl, key, i);
	if (ret)
		fail("nvl_set_int(..., '%s', %"PRIu64") failed", key, i);
}

static inline void set_null(struct nvlist *nvl, const char *key)
{
	int ret;

	ret = nvl_set_null(nvl, key);
	if (ret)
		fail("nvl_set_null(..., '%s') failed", key);
}

static inline void __check_lookup_err(bool iserr, bool expected_iserr,
				      int err, int expected_err,
				      const char *fxn, const char *key)
{
	if (!iserr && !expected_iserr)
		return;

	if (iserr && !expected_iserr)
		fail("%s(..., '%s') failed (%s), expected success",
		     fxn, key, xstrerror(err));

	if (!iserr && expected_iserr)
		fail("%s(..., '%s') succeded, expected failure (%s)",
		     fxn, key, xstrerror(expected_err));

	if (err != expected_err)
		fail("%s(..., '%s') returned %s (%d), expected %s (%d)",
		     fxn, key, xstrerror(err), err, xstrerror(expected_err),
		     expected_err);
}

static void check_key_not_exists(struct nvlist *nvl, const char *key)
{
	const struct nvpair *pair;
	struct val **vals;
	struct nvlist *cnvl;
	size_t nelem, size;
	const void *ptr;
	struct str *str;
	uint64_t int_val;
	bool bool_val;
	int ret;

	ret = nvl_lookup_array(nvl, key, &vals, &nelem);
	__check_lookup_err(!!ret, true, ret, -ENOENT, "nvl_lookup_array", key);

	ret = nvl_lookup_blob(nvl, key, &ptr, &size);
	__check_lookup_err(!!ret, true, ret, -ENOENT, "nvl_lookup_blob", key);

	ret = nvl_lookup_bool(nvl, key, &bool_val);
	__check_lookup_err(!!ret, true, ret, -ENOENT, "nvl_lookup_bool", key);

	ret = nvl_lookup_int(nvl, key, &int_val);
	__check_lookup_err(!!ret, true, ret, -ENOENT, "nvl_lookup_int", key);

	ret = nvl_lookup_null(nvl, key);
	__check_lookup_err(!!ret, true, ret, -ENOENT, "nvl_lookup_null", key);

	pair = nvl_lookup(nvl, key);
	__check_lookup_err(IS_ERR(pair), true, PTR_ERR(pair), -ENOENT,
			   "nvl_lookup", key);

	cnvl = nvl_lookup_nvl(nvl, key);
	__check_lookup_err(IS_ERR(cnvl), true, PTR_ERR(cnvl), -ENOENT,
			   "nvl_lookup_nvl", key);

	str = nvl_lookup_str(nvl, key);
	__check_lookup_err(IS_ERR(str), true, PTR_ERR(str), -ENOENT,
			   "nvl_lookup_str", key);
}

static void check_key_exists(struct nvlist *nvl, const char *key,
			     enum val_type type)
{
	const struct nvpair *pair;
	struct val **vals;
	struct nvlist *cnvl;
	size_t nelem, size;
	const void *ptr;
	struct str *str;
	uint64_t int_val;
	bool bool_val;
	int ret;

	pair = nvl_lookup(nvl, key);
	__check_lookup_err(IS_ERR(pair), false, PTR_ERR(pair), 0, "nvl_lookup",
			   key);

	if (pair->value->type != type)
		fail("nvl_lookup(..., '%s') returned wrong type; got %u, "
		     "expected %u", key, pair->value->type, type);

	ret = nvl_lookup_array(nvl, key, &vals, &nelem);
	__check_lookup_err(!!ret, type != VT_ARRAY, ret, -ERANGE,
			   "nvl_lookup_array", key);

	ret = nvl_lookup_blob(nvl, key, &ptr, &size);
	__check_lookup_err(!!ret, type != VT_BLOB, ret, -ERANGE,
			   "nvl_lookup_blob", key);

	ret = nvl_lookup_bool(nvl, key, &bool_val);
	__check_lookup_err(!!ret, type != VT_BOOL, ret, -ERANGE,
			   "nvl_lookup_bool", key);

	ret = nvl_lookup_int(nvl, key, &int_val);
	__check_lookup_err(!!ret, type != VT_INT, ret, -ERANGE,
			   "nvl_lookup_int", key);

	ret = nvl_lookup_null(nvl, key);
	__check_lookup_err(!!ret, type != VT_NULL, ret, -ERANGE,
			   "nvl_lookup_null", key);

	cnvl = nvl_lookup_nvl(nvl, key);
	__check_lookup_err(IS_ERR(cnvl), type != VT_NVL, PTR_ERR(cnvl),
			   -ERANGE, "nvl_lookup_nvl", key);

	str = nvl_lookup_str(nvl, key);
	__check_lookup_err(IS_ERR(str), type != VT_STR, PTR_ERR(str),
			   -ERANGE, "nvl_lookup_str", key);
}

static inline void check_empty(struct nvlist *nvl)
{
	const struct nvpair *pair;

	pair = nvl_iter_start(nvl);

	if (pair)
		fail("nvlist %p not empty");
}

static void test_alloc_free(void)
{
	struct nvlist *nvl;

	fprintf(stderr, "%s...", __func__);

	nvl = alloc();

	check_empty(nvl);

	nvl_putref(nvl);

	fprintf(stderr, "ok.\n");
}

static void test_refs(void)
{
	struct nvlist *nvl;

	fprintf(stderr, "%s...", __func__);

	nvl = alloc();

	nvl_getref(nvl);
	check_empty(nvl);
	nvl_putref(nvl);
	check_empty(nvl);

	nvl_getref(nvl);
	check_empty(nvl);
	nvl_putref(nvl);
	check_empty(nvl);

	nvl_putref(nvl);

	fprintf(stderr, "ok.\n");
}

static void test_lookup_empty(void)
{
	struct nvlist *nvl;

	fprintf(stderr, "%s...", __func__);

	nvl = alloc();

	check_key_not_exists(nvl, "non-existent");

	nvl_putref(nvl);

	fprintf(stderr, "ok.\n");
}

static void test_lookup_simple(void)
{
	struct nvlist *nvl;

	fprintf(stderr, "%s...", __func__);

	nvl = alloc();

	check_key_not_exists(nvl, "non-existent");

	set_int(nvl, "abc", 1);

	check_key_not_exists(nvl, "non-existent");
	check_key_exists(nvl, "abc", VT_INT);

	set_bool(nvl, "def", true);

	check_key_not_exists(nvl, "non-existent");
	check_key_exists(nvl, "abc", VT_INT);
	check_key_exists(nvl, "def", VT_BOOL);

	set_null(nvl, "ghi");

	check_key_not_exists(nvl, "non-existent");
	check_key_exists(nvl, "abc", VT_INT);
	check_key_exists(nvl, "def", VT_BOOL);
	check_key_exists(nvl, "ghi", VT_NULL);

	nvl_putref(nvl);

	fprintf(stderr, "ok.\n");
}

static inline void check_merge_set(struct nvlist *nvl, unsigned imask,
				   unsigned bmask)
{
	static const char *names[] = {
		"abc", "def", "ghi", "jkl",
	};
	const unsigned mask = imask | bmask;
	int i;

	for (i = 0; i < 4; i++) {
		const char *name = names[i];
		unsigned bit = 1 << i;

		if ((mask & bit) == 0)
			check_key_not_exists(nvl, name);
		else if (imask & bit)
			check_key_exists(nvl, name, VT_INT);
		else if (bmask & bit)
			check_key_exists(nvl, name, VT_BOOL);
	}
}

static void test_merge(void)
{
	struct nvlist *a, *b;

	fprintf(stderr, "%s...", __func__);

	a = alloc();
	b = alloc();

	check_merge_set(a, 0, 0);
	check_merge_set(b, 0, 0);

	fprintf(stderr, "1...");

	set_int(a, "abc", 1);
	set_int(a, "ghi", 2);
	set_bool(b, "abc", true);
	set_int(b, "def", 4);

	fprintf(stderr, "2...");

	check_merge_set(a, 0x5, 0x0);
	check_merge_set(b, 0x2, 0x1);

	fprintf(stderr, "3...");

	merge(a, b);

	fprintf(stderr, "4...");

	check_merge_set(a, 0x6, 0x1);
	check_merge_set(b, 0x2, 0x1);

	fprintf(stderr, "5...");

	merge(a, b);

	fprintf(stderr, "6...");

	check_merge_set(a, 0x6, 0x1);
	check_merge_set(b, 0x2, 0x1);

	fprintf(stderr, "7...");

	merge(b, a);

	fprintf(stderr, "8...");

	check_merge_set(a, 0x6, 0x1);
	check_merge_set(b, 0x6, 0x1);

	fprintf(stderr, "9...");

	nvl_putref(a);
	nvl_putref(b);

	fprintf(stderr, "ok.\n");
}

void test(void)
{
	test_alloc_free();
	test_refs();
	test_lookup_empty();
	test_lookup_simple();
	test_merge();
}
