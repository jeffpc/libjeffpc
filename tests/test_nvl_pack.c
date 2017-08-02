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
#include <jeffpc/hexdump.h>

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
	struct {
		const uint8_t *data;
		size_t len;
	} cbor;
};

#define C(type, addl)	((uint8_t)(((type) << 5) | (addl)))

#define CBOR_INT1(v)	C(0, v)
#define CBOR_INT2(v)	C(0, 24), (v)
#define CBOR_INT3(v)	C(0, 25), ((v) >> 8), ((v) & 0xff)
#define CBOR_INT5(v)	C(0, 26), ((v) >> 24), (((v) >> 16) & 0xff), \
			(((v) >> 8) & 0xff), ((v) & 0xff)
#define CBOR_INT9(v)	C(0, 27), \
			((v) >> 56), (((v) >> 48) & 0xff), \
			(((v) >> 40) & 0xff), (((v) >> 32) & 0xff), \
			(((v) >> 24) & 0xff), (((v) >> 16) & 0xff), \
			(((v) >> 8) & 0xff), ((v) & 0xff)
#define CBOR_STR1(len)	C(3, len)
#define CBOR_MAP_START	C(5, 31)
#define CBOR_FALSE	C(7, 20)
#define CBOR_TRUE	C(7, 21)
#define CBOR_NULL	C(7, 22)
#define CBOR_BREAK	C(7, 31)

#define CBOR(name)						\
	{							\
		.data = name,					\
		.len = sizeof(name),				\
	}

static const uint8_t cbor_empty[] = {
	CBOR_MAP_START,
	CBOR_BREAK,
};
static const uint8_t cbor_abc5[]  = {
	CBOR_MAP_START,
		CBOR_STR1(3), 'a', 'b', 'c',
		CBOR_INT1(5),
	CBOR_BREAK,
};
static const uint8_t cbor_abc5_deftrue[]  = {
	CBOR_MAP_START,
		CBOR_STR1(3), 'a', 'b', 'c',
		CBOR_INT1(5),
		CBOR_STR1(3), 'd', 'e', 'f',
		CBOR_TRUE,
	CBOR_BREAK,
};
static const uint8_t cbor_deftrue[]  = {
	CBOR_MAP_START,
		CBOR_STR1(3), 'd', 'e', 'f',
		CBOR_TRUE,
	CBOR_BREAK,
};
static const uint8_t cbor_deftrue_abcnull[]  = {
	CBOR_MAP_START,
		CBOR_STR1(3), 'd', 'e', 'f',
		CBOR_TRUE,
		CBOR_STR1(3), 'a', 'b', 'c',
		CBOR_NULL,
	CBOR_BREAK,
};
static const uint8_t cbor_deftrue_abcfalse[]  = {
	CBOR_MAP_START,
		CBOR_STR1(3), 'd', 'e', 'f',
		CBOR_TRUE,
		CBOR_STR1(3), 'a', 'b', 'c',
		CBOR_FALSE,
	CBOR_BREAK,
};
static const uint8_t cbor_a0[]  = {
	CBOR_MAP_START,
		CBOR_STR1(1), 'a',
		CBOR_INT1(0),
	CBOR_BREAK,
};
static const uint8_t cbor_a0_b23[]  = {
	CBOR_MAP_START,
		CBOR_STR1(1), 'a',
		CBOR_INT1(0),
		CBOR_STR1(1), 'b',
		CBOR_INT1(23),
	CBOR_BREAK,
};
static const uint8_t cbor_a0_b23_c24[]  = {
	CBOR_MAP_START,
		CBOR_STR1(1), 'a',
		CBOR_INT1(0),
		CBOR_STR1(1), 'b',
		CBOR_INT1(23),
		CBOR_STR1(1), 'c',
		CBOR_INT2(24),
	CBOR_BREAK,
};
static const uint8_t cbor_a0_b23_c24_d255[]  = {
	CBOR_MAP_START,
		CBOR_STR1(1), 'a',
		CBOR_INT1(0),
		CBOR_STR1(1), 'b',
		CBOR_INT1(23),
		CBOR_STR1(1), 'c',
		CBOR_INT2(24),
		CBOR_STR1(1), 'd',
		CBOR_INT2(255),
	CBOR_BREAK,
};
static const uint8_t cbor_a0_b23_c24_d255_e256[]  = {
	CBOR_MAP_START,
		CBOR_STR1(1), 'a',
		CBOR_INT1(0),
		CBOR_STR1(1), 'b',
		CBOR_INT1(23),
		CBOR_STR1(1), 'c',
		CBOR_INT2(24),
		CBOR_STR1(1), 'd',
		CBOR_INT2(255),
		CBOR_STR1(1), 'e',
		CBOR_INT3(256),
	CBOR_BREAK,
};
static const uint8_t cbor_a0_b23_c24_d255_e256_f65535[]  = {
	CBOR_MAP_START,
		CBOR_STR1(1), 'a',
		CBOR_INT1(0),
		CBOR_STR1(1), 'b',
		CBOR_INT1(23),
		CBOR_STR1(1), 'c',
		CBOR_INT2(24),
		CBOR_STR1(1), 'd',
		CBOR_INT2(255),
		CBOR_STR1(1), 'e',
		CBOR_INT3(256),
		CBOR_STR1(1), 'f',
		CBOR_INT3(65535),
	CBOR_BREAK,
};
static const uint8_t cbor_a0_b23_c24_d255_e256_f65535_g65536[]  = {
	CBOR_MAP_START,
		CBOR_STR1(1), 'a',
		CBOR_INT1(0),
		CBOR_STR1(1), 'b',
		CBOR_INT1(23),
		CBOR_STR1(1), 'c',
		CBOR_INT2(24),
		CBOR_STR1(1), 'd',
		CBOR_INT2(255),
		CBOR_STR1(1), 'e',
		CBOR_INT3(256),
		CBOR_STR1(1), 'f',
		CBOR_INT3(65535),
		CBOR_STR1(1), 'g',
		CBOR_INT5(65536),
	CBOR_BREAK,
};
static const uint8_t cbor_a0_b23_c24_d255_e256_f65535_g65536_h4294967295[]  = {
	CBOR_MAP_START,
		CBOR_STR1(1), 'a',
		CBOR_INT1(0),
		CBOR_STR1(1), 'b',
		CBOR_INT1(23),
		CBOR_STR1(1), 'c',
		CBOR_INT2(24),
		CBOR_STR1(1), 'd',
		CBOR_INT2(255),
		CBOR_STR1(1), 'e',
		CBOR_INT3(256),
		CBOR_STR1(1), 'f',
		CBOR_INT3(65535),
		CBOR_STR1(1), 'g',
		CBOR_INT5(65536),
		CBOR_STR1(1), 'h',
		CBOR_INT5(0xffffffff),
	CBOR_BREAK,
};
static const uint8_t cbor_a0_b23_c24_d255_e256_f65535_g65536_h4294967295_i4294967296[]  = {
	CBOR_MAP_START,
		CBOR_STR1(1), 'a',
		CBOR_INT1(0),
		CBOR_STR1(1), 'b',
		CBOR_INT1(23),
		CBOR_STR1(1), 'c',
		CBOR_INT2(24),
		CBOR_STR1(1), 'd',
		CBOR_INT2(255),
		CBOR_STR1(1), 'e',
		CBOR_INT3(256),
		CBOR_STR1(1), 'f',
		CBOR_INT3(65535),
		CBOR_STR1(1), 'g',
		CBOR_INT5(65536),
		CBOR_STR1(1), 'h',
		CBOR_INT5(0xffffffff),
		CBOR_STR1(1), 'i',
		CBOR_INT9(0x100000000),
	CBOR_BREAK,
};
static const uint8_t cbor_a1311768467463790320[] = {
	CBOR_MAP_START,
		CBOR_STR1(1), 'a',
		CBOR_INT9(0x123456789abcdef0),
	CBOR_BREAK,
};
static const uint8_t cbor_a305419896[] = {
	CBOR_MAP_START,
		CBOR_STR1(1), 'a',
		CBOR_INT5(0x12345678),
	CBOR_BREAK,
};
static const uint8_t cbor_a4660[] = {
	CBOR_MAP_START,
		CBOR_STR1(1), 'a',
		CBOR_INT3(0x1234),
	CBOR_BREAK,
};

static const struct test tests[] = {
	{
		.action = NOP,
		.json = "{}",
		.cbor = CBOR(cbor_empty),
	},
	{
		.action = REALLOC,
		.json = "{}",
		.cbor = CBOR(cbor_empty),
	},
	{
		.action = UNSET,
		.name = "abc",
		.ret = -ENOENT,
		.json = "{}",
		.cbor = CBOR(cbor_empty),
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
		.cbor = CBOR(cbor_abc5),
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
		.cbor = CBOR(cbor_abc5_deftrue),
	},
	{
		.action = UNSET,
		.name = "abc",
		.ret = 0,
		.json = "{\"def\":true}",
		.cbor = CBOR(cbor_deftrue),
	},
	{
		.action = SET,
		.name = "abc",
		.val = {
			.type = NVT_NULL,
		},
		.ret = 0,
		.json = "{\"def\":true,\"abc\":null}",
		.cbor = CBOR(cbor_deftrue_abcnull),
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
		.cbor = CBOR(cbor_deftrue_abcfalse),
	},
	{
		.action = REALLOC,
		.json = "{}",
		.cbor = CBOR(cbor_empty),
	},
	{
		.action = SET,
		.name = "a",
		.val = {
			.type = NVT_INT,
			.i = 0,
		},
		.json = "{\"a\":0}",
		.cbor = CBOR(cbor_a0),
	},
	{
		.action = SET,
		.name = "b",
		.val = {
			.type = NVT_INT,
			.i = 23,
		},
		.json = "{\"a\":0,\"b\":23}",
		.cbor = CBOR(cbor_a0_b23),
	},
	{
		.action = SET,
		.name = "c",
		.val = {
			.type = NVT_INT,
			.i = 24,
		},
		.json = "{\"a\":0,\"b\":23,\"c\":24}",
		.cbor = CBOR(cbor_a0_b23_c24),
	},
	{
		.action = SET,
		.name = "d",
		.val = {
			.type = NVT_INT,
			.i = 255,
		},
		.json = "{\"a\":0,\"b\":23,\"c\":24,\"d\":255}",
		.cbor = CBOR(cbor_a0_b23_c24_d255),
	},
	{
		.action = SET,
		.name = "e",
		.val = {
			.type = NVT_INT,
			.i = 256,
		},
		.json = "{\"a\":0,\"b\":23,\"c\":24,\"d\":255,\"e\":256}",
		.cbor = CBOR(cbor_a0_b23_c24_d255_e256),
	},
	{
		.action = SET,
		.name = "f",
		.val = {
			.type = NVT_INT,
			.i = 0xffff,
		},
		.json = "{"
			"\"a\":0,\"b\":23,\"c\":24,\"d\":255,\"e\":256,"
			"\"f\":65535"
			"}",
		.cbor = CBOR(cbor_a0_b23_c24_d255_e256_f65535),
	},
	{
		.action = SET,
		.name = "g",
		.val = {
			.type = NVT_INT,
			.i = 0x10000,
		},
		.json = "{"
			"\"a\":0,\"b\":23,\"c\":24,\"d\":255,\"e\":256,"
			"\"f\":65535,\"g\":65536"
			"}",
		.cbor = CBOR(cbor_a0_b23_c24_d255_e256_f65535_g65536),
	},
	{
		.action = SET,
		.name = "h",
		.val = {
			.type = NVT_INT,
			.i = 0xffffffff,
		},
		.json = "{"
			"\"a\":0,\"b\":23,\"c\":24,\"d\":255,\"e\":256,"
			"\"f\":65535,\"g\":65536,\"h\":4294967295"
			"}",
		.cbor = CBOR(cbor_a0_b23_c24_d255_e256_f65535_g65536_h4294967295),
	},
	{
		.action = SET,
		.name = "i",
		.val = {
			.type = NVT_INT,
			.i = 0x100000000,
		},
		.json = "{"
			"\"a\":0,\"b\":23,\"c\":24,\"d\":255,\"e\":256,"
			"\"f\":65535,\"g\":65536,\"h\":4294967295,"
			"\"i\":4294967296"
			"}",
		.cbor = CBOR(cbor_a0_b23_c24_d255_e256_f65535_g65536_h4294967295_i4294967296),
	},
	{
		.action = REALLOC,
		.json = "{}",
		.cbor = CBOR(cbor_empty),
	},
	{
		.action = SET,
		.name = "a",
		.val = {
			.type = NVT_INT,
			.i = 0x123456789abcdef0,
		},
		.json = "{\"a\":1311768467463790320}",
		.cbor = CBOR(cbor_a1311768467463790320),
	},
	{
		.action = SET,
		.name = "a",
		.val = {
			.type = NVT_INT,
			.i = 0x12345678,
		},
		.json = "{\"a\":305419896}",
		.cbor = CBOR(cbor_a305419896),
	},
	{
		.action = SET,
		.name = "a",
		.val = {
			.type = NVT_INT,
			.i = 0x1234,
		},
		.json = "{\"a\":4660}",
		.cbor = CBOR(cbor_a4660),
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

static inline void hexdump_buf(const void *in, size_t len)
{
	char tmp[len * 2 + 1];

	hexdumpz(tmp, in, len, false);

	fprintf(stderr, "%s", tmp);
}

static inline void check_packing(struct nvlist *nvl, const struct test *test)
{
	struct buffer *buf;

	/* JSON */
	buf = nvl_pack(nvl, NVF_JSON);
	fprintf(stderr, "  JSON expected: %s\n", test->json);
	fprintf(stderr, "  JSON got:      %s\n", (const char *) buffer_data(buf));

	if (buffer_used(buf) != strlen(test->json))
		fail("json packing failed: length mismatch "
		     "(got %zu, expected %zu)", buffer_used(buf),
		     strlen(test->json));

	if (memcmp(buffer_data(buf), test->json, buffer_used(buf)))
		fail("json packing failed: content mismatch");

	buffer_free(buf);

	/* CBOR */
	buf = nvl_pack(nvl, NVF_CBOR);
	fprintf(stderr, "  CBOR expected: ");
	hexdump_buf(test->cbor.data, test->cbor.len);
	fprintf(stderr, "\n");
	fprintf(stderr, "  CBOR got:      ");
	hexdump_buf(buffer_data(buf), buffer_used(buf));
	fprintf(stderr, "\n");

	if (buffer_used(buf) != test->cbor.len)
		fail("cbor packing failed (len %zu, expected %zu)",
		     buffer_used(buf), test->cbor.len);

	if (memcmp(buffer_data(buf), test->cbor.data, test->cbor.len))
		fail("cbor packing failed");

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
