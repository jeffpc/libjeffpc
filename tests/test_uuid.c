/*
 * Copyright (c) 2018 Josef 'Jeff' Sipek <jeffpc@josefsipek.net>
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

#include <jeffpc/types.h>
#include <jeffpc/error.h>
#include <jeffpc/uuid.h>
#include <jeffpc/hexdump.h>

#include "test.c"

struct test {
	const char *str;
	struct xuuid bin;
	bool parse_ret;
};

static const struct test tests[] = {
	{
		/* extra char at the end */
		.str = "9e9a19f3-fb85-6c3d-9a91-b5a9efee2d970",
		.parse_ret = false,
	},
	{
		/* not enough chars */
		.str = "9e9a19f3-fb85-6c3d-9a91-b5a9efee2d9",
		.parse_ret = false,
	},
	{
		/* missing dashes */
		.str = "9e9a19f3fb856c3d9a91b5a9efee2d97",
		.parse_ret = false,
	},
	{
		/* misplaced dash 1 */
		.str = "9e9a19f3f-b85-6c3d-9a91-b5a9efee2d9",
		.parse_ret = false,
	},
	{
		/* misplaced dash 1 */
		.str = "9e9a19f-3fb85-6c3d-9a91-b5a9efee2d9",
		.parse_ret = false,
	},
	{
		/* misplaced dash 2 */
		.str = "9e9a19f3-fb856-c3d-9a91-b5a9efee2d9",
		.parse_ret = false,
	},
	{
		/* misplaced dash 2 */
		.str = "9e9a19f3-fb8-56c3d-9a91-b5a9efee2d9",
		.parse_ret = false,
	},
	{
		/* misplaced dash 3 */
		.str = "9e9a19f3-fb85-6c3d9-a91-b5a9efee2d9",
		.parse_ret = false,
	},
	{
		/* misplaced dash 3 */
		.str = "9e9a19f3-fb85-6c3-da91b-5a9efee2d9",
		.parse_ret = false,
	},
	{
		/* misplaced dash 4 */
		.str = "9e9a19f3-fb85-6c3d-9a91b-5a9efee2d9",
		.parse_ret = false,
	},
	{
		/* misplaced dash 4 */
		.str = "9e9a19f3-fb85-6c3d-9a9-1b5a9efee2d9",
		.parse_ret = false,
	},
	{
		.str = "00000000-0000-0000-0000-000000000000",
		.bin = { { 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
			   0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, } },
		.parse_ret = true,
	},
	{
		.str = "9e9a19f3-fb85-6c3d-9a91-b5a9efee2d97",
		.bin = { { 0x9e, 0x9a, 0x19, 0xf3, 0xfb, 0x85, 0x6c, 0x3d,
			   0x9a, 0x91, 0xb5, 0xa9, 0xef, 0xee, 0x2d, 0x97, } },
		.parse_ret = true,
	},
	{
		.str = "9E9A19F3-FB85-6C3D-9A91-B5A9EFEE2D97",
		.bin = { { 0x9e, 0x9a, 0x19, 0xf3, 0xfb, 0x85, 0x6c, 0x3d,
			   0x9a, 0x91, 0xb5, 0xa9, 0xef, 0xee, 0x2d, 0x97, } },
		.parse_ret = true,
	},
	{
		.str = "9e9a19F3-fB85-6C3D-9A91-b5a9eFeE2D97",
		.bin = { { 0x9e, 0x9a, 0x19, 0xf3, 0xfb, 0x85, 0x6c, 0x3d,
			   0x9a, 0x91, 0xb5, 0xa9, 0xef, 0xee, 0x2d, 0x97, } },
		.parse_ret = true,
	},
};

static void test_null_const(void)
{
	struct xuuid empty = { { 0 } };

	fprintf(stderr, "%s: ", __func__);
	if (memcmp(&xuuid_null_uuid, &empty, sizeof(struct xuuid))) {
		char dump_got[sizeof(struct xuuid) * 2 + 1];

		hexdumpz(dump_got, &xuuid_null_uuid, sizeof(struct xuuid), false);
		fail("xuuid_null_uuid isn't null (got: %s)", dump_got);
	}
	fprintf(stderr, "ok.\n");
}

static void test_parse_unparse(void)
{
	int i;

	for (i = 0; i < ARRAY_LEN(tests); i++) {
		const struct test *test = &tests[i];
		char tmp_str[XUUID_PRINTABLE_STRING_LENGTH];
		struct xuuid tmp_uuid;
		bool ret;

		fprintf(stderr, "%s: iter = %2d...", __func__, i);

		ret = xuuid_parse(&tmp_uuid, test->str);
		if (ret != test->parse_ret)
			fail("parse returned %s, expected %s",
			     ret ? "true" : "false",
			     test->parse_ret ? "true" : "false");

		/* if parse failed, there's nothing else to do for this test */
		if (!ret) {
			fprintf(stderr, "ok.\n");
			continue;
		}

		if (xuuid_compare(&tmp_uuid, &test->bin)) {
			char dump_exp[sizeof(struct xuuid) * 2 + 1];
			char dump_got[sizeof(struct xuuid) * 2 + 1];

			hexdumpz(dump_exp, &test->bin, sizeof(struct xuuid), false);
			hexdumpz(dump_got, &tmp_uuid, sizeof(struct xuuid), false);
			fail("parsed uuid doesn't match (exp: %s, got: %s)",
			     dump_exp, dump_got);
		}

		xuuid_unparse(&test->bin, tmp_str);

		if (strcasecmp(tmp_str, test->str))
			fail("unparsed uuid doesn't match original (exp: %s, got: %s)",
			     test->str, tmp_str);

		fprintf(stderr, "ok.\n");
	}
}

void test(void)
{
	test_null_const();
	test_parse_unparse();
}
