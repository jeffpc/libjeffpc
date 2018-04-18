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

#include <jeffpc/sexpr.h>

#include "test.c"

static const struct test {
	const char *in;
	enum val_type type;
	bool same;
} tests[] = {
	{
		.in = "()",
		.type = VT_CONS,
		.same = true,
	},
	{
		.in = "(1 2 3)",
		.type = VT_ARRAY,
		.same = false,
	},
	{
		.in = "((\"a\" . b) (\"c\" . d))",
		.type = VT_NVL,
		.same = false,
	},
	{
		.in = "((a . b) (c . d))",
		.type = VT_NVL,
		.same = false,
	},
	{
		.in = "((a b) . (c d))",
		.type = VT_ARRAY,
		.same = false,
	},
	{
		.in = "((a b) . d)",
		.type = VT_CONS,
		.same = true,
	},
};

void test(void)
{
	size_t i;

	for (i = 0; i < ARRAY_LEN(tests); i++) {
		const struct test *run = &tests[i];
		struct val *in;
		struct val *out;

		fprintf(stderr, "%2zu: %s, expecting %s %s...\n", i, run->in,
			run->same ? "identical" : "new",
			val_typename(run->type));

		in = sexpr_parse(run->in, strlen(run->in));
		ASSERT(!IS_ERR(in));

		fprintf(stderr, "    ");
		sexpr_dump_file(stderr, in, false);
		fprintf(stderr, " <== parsed/reformated sexpr\n");

		out = sexpr_compact(val_getref(in));
		ASSERT(!IS_ERR(out));

		val_dump_file(stderr, out, 1);

		if (run->same && (in != out))
			fail("compaction was not a no-op");
		if (!run->same && (in == out))
			fail("compaction was a no-op");

		if (out->type != run->type)
			fail("compaction returned wrong type "
			     "(exp: %s, got: %s)",
			     val_typename(run->type),
			     val_typename(out->type));

		val_putref(in);
		val_putref(out);

		fprintf(stderr, "%2zu: ok.\n", i);
	}
}
