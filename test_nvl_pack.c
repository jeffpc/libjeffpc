/*
 * Copyright (c) 2017 Josef 'Jeff' Sipek <jeffpc@josefsipek.net>
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

struct test {
	enum {
		NOP,
		SET,
		UNSET,
		REALLOC,	/* empties the nvl */
	} action;

	const char *name;
	struct nvval val;
	int ret;

	/* expected outputs */
	const char *json;
};

static const struct test tests[] = {
	{
		.action = NOP,
		.json = "{}",
	},
	{
		.action = REALLOC,
		.json = "{}",
	},
	{
		.action = UNSET,
		.name = "abc",
		.ret = -ENOENT,
		.json = "{}",
	},
	{
		.action = SET,
		.name = "abc",
		.val = {
			.type = NVT_INT,
			.i = 5,
		},
		.ret = 0,
		.json = "{\"abc\":5}",
	},
	{
		.action = SET,
		.name = "def",
		.val = {
			.type = NVT_BOOL,
			.b = true,
		},
		.ret = 0,
		.json = "{\"abc\":5,\"def\":true}",
	},
	{
		.action = UNSET,
		.name = "abc",
		.ret = 0,
		.json = "{\"def\":true}",
	},
	{
		.action = SET,
		.name = "abc",
		.val = {
			.type = NVT_NULL,
		},
		.ret = 0,
		.json = "{\"def\":true,\"abc\":null}",
	},
	{
		.action = SET,
		.name = "abc",
		.val = {
			.type = NVT_BOOL,
			.b = false,
		},
		.ret = 0,
		.json = "{\"def\":true,\"abc\":false}",
	},
};

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

static inline void set(struct nvlist *nvl, const char *key,
		       const struct nvval *val, int expected_ret)
{
	switch (val->type) {
		case NVT_ARRAY:
		case NVT_BLOB:
			fail("not yet implemented");
		case NVT_BOOL:
			set_bool(nvl, key, val->b, expected_ret);
			break;
		case NVT_INT:
			set_int(nvl, key, val->i, expected_ret);
			break;
		case NVT_NULL:
			set_null(nvl, key, expected_ret);
			break;
		case NVT_NVL:
		case NVT_STR:
			fail("not yet implemented");
	}
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

static inline void check_packing(struct nvlist *nvl, const struct test *test)
{
	struct buffer *buf;

	buf = nvl_pack(nvl, NVF_JSON);
	fprintf(stderr, "  expected: %s\n", test->json);
	fprintf(stderr, "  got:      %s\n", (const char *) buffer_data(buf));

	if (strcmp(buffer_data(buf), test->json))
		fail("json packing failed");

	buffer_free(buf);
}

void test(void)
{
	struct nvlist *nvl;
	int i;

	nvl = alloc();

	for (i = 0; i < ARRAY_LEN(tests); i++) {
		const struct test *test = &tests[i];

		fprintf(stderr, "%s: iter=%d ", __func__, i);

		switch (test->action) {
			case NOP:
				fprintf(stderr, "(NOP)...");
				break;
			case SET:
				fprintf(stderr, "(SET %s)...", test->name);
				set(nvl, test->name, &test->val, test->ret);
				break;
			case UNSET:
				fprintf(stderr, "(UNSET %s)...", test->name);
				unset(nvl, test->name, test->ret);
				break;
			case REALLOC:
				fprintf(stderr, "(REALLOC)...");
				nvl_putref(nvl);
				nvl = alloc();
				break;
		}

		fprintf(stderr, "\n");

		check_packing(nvl, test);

		fprintf(stderr, "ok.\n");
	}

	nvl_putref(nvl);
}
