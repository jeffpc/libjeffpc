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

#include <jeffpc/types.h>

#include "test.c"

struct foo {
	int a;
	char b;
	double c;
	int d[5];
	long e;
};

#define CHECK(x, y) \
	do { \
		fprintf(stderr, "%s == %s ...", #x, #y); \
		if ((x) != (y)) \
			fail("failed"); \
		fprintf(stderr, "ok.\n"); \
	} while (0)

#define CHECK_BAD(x, y) \
	do { \
		fprintf(stderr, "%s != %s ...", #x, #y); \
		if ((x) == (y)) \
			fail("failed"); \
		fprintf(stderr, "ok.\n"); \
	} while (0)

void test(void)
{
	struct foo foo;
	struct foo bar;

	CHECK(container_of(&foo.a, struct foo, a), &foo);
	CHECK(container_of(&foo.b, struct foo, b), &foo);
	CHECK(container_of(&foo.c, struct foo, c), &foo);
	CHECK(container_of(&foo.d, struct foo, d), &foo);
	CHECK(container_of(&foo.e, struct foo, e), &foo);

	CHECK_BAD(container_of(&foo.b, struct foo, a), &foo);
	CHECK_BAD(container_of(&foo.c, struct foo, a), &foo);
	CHECK_BAD(container_of(&foo.d, struct foo, a), &foo);
	CHECK_BAD(container_of(&foo.e, struct foo, a), &foo);

	CHECK_BAD(container_of(&foo.a, struct foo, b), &foo);
	CHECK_BAD(container_of(&foo.c, struct foo, b), &foo);
	CHECK_BAD(container_of(&foo.d, struct foo, b), &foo);
	CHECK_BAD(container_of(&foo.e, struct foo, b), &foo);

	CHECK_BAD(container_of(&foo.a, struct foo, c), &foo);
	CHECK_BAD(container_of(&foo.b, struct foo, c), &foo);
	CHECK_BAD(container_of(&foo.d, struct foo, c), &foo);
	CHECK_BAD(container_of(&foo.e, struct foo, c), &foo);

	CHECK_BAD(container_of(&foo.a, struct foo, d), &foo);
	CHECK_BAD(container_of(&foo.b, struct foo, d), &foo);
	CHECK_BAD(container_of(&foo.c, struct foo, d), &foo);
	CHECK_BAD(container_of(&foo.e, struct foo, d), &foo);

	CHECK_BAD(container_of(&foo.a, struct foo, e), &foo);
	CHECK_BAD(container_of(&foo.b, struct foo, e), &foo);
	CHECK_BAD(container_of(&foo.c, struct foo, e), &foo);
	CHECK_BAD(container_of(&foo.d, struct foo, e), &foo);

	CHECK_BAD(container_of(&foo.a, struct foo, a), &bar);
}
