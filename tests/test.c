/*
 * Copyright (c) 2016-2019 Josef 'Jeff' Sipek <jeffpc@josefsipek.net>
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

#include <stdio.h>
#include <stdarg.h>
#include <stdlib.h>
#include <string.h>

#include <jeffpc/jeffpc.h>
#include <jeffpc/version.h>
#include <jeffpc/error.h>

#ifndef USE_FILENAME_ARGS
static void test(void);
#else
static void test(const char *);
#endif

static const char *expected_panic_string;

/*
 * Do not make this static or the compiler may complain about an unused
 * static function.
 */
void NORETURN fail(const char *fmt, ...)
{
	va_list ap;

	fprintf(stderr, "TEST FAILED: ");

	va_start(ap, fmt);
	vfprintf(stderr, fmt, ap);
	va_end(ap);

	fprintf(stderr, "\n");

	exit(1);
}

void test_set_panic_string(const char *str)
{
	expected_panic_string = str;
}

/*
 * We intercept the cmn_err printing to force all output to stderr, to
 * ensure everything was flushed, and to handle expected panics.  Usually
 * stderr is unbuffered, but these are tests so (1) we expect errors, (2) we
 * really don't want to lose them, and (3) performance isn't that important.
 */
static void test_print(enum errlevel level, const char *fmt, va_list ap)
{
	char buf[4096];
	size_t len;

	len = vsnprintf(buf, sizeof(buf), fmt, ap);
	buf[sizeof(buf) - 1] = '\0'; /* ensure nul-termination */

	fwrite(buf, len, 1, stderr);
	fflush(stderr);

	if ((level == CE_PANIC) && expected_panic_string &&
	    strstr(buf, expected_panic_string)) {
		/*
		 * We got a panic, expected a panic, and the panic string
		 * matches the substring we were given - all is good.  Let's
		 * terminate the test gracefully.
		 *
		 * We print a message to make it harder to misinterpret the
		 * above printed panic message.
		 */
		fprintf(stderr, "Expected panic string matched - Test passed.\n");
		exit(0);
	}

	/*
	 * If we had an unexpected panic, cmd_err will end up aborting the
	 * process causing the test to fail.
	 */
}

int main(int argc, char **argv)
{
	struct jeffpc_ops init_ops = {
		.print = test_print,
	};

	fprintf(stderr, "libjeffpc.so version %s\n", jeffpc_version);
	fprintf(stderr, "Running tests (%s)\n", argv[0]);

#ifndef USE_FILENAME_ARGS
	fprintf(stderr, "Not expecteding any args\n");
#else
	fprintf(stderr, "Expecting filename args\n");
#endif

	jeffpc_init(&init_ops);

#ifndef USE_FILENAME_ARGS
	test();
#else
	for (int i = 1; i < argc; i++) {
		fprintf(stderr, "Checking %s...\n", argv[i]);

		test(argv[i]);
	}
#endif

	/* no panics encountered */

	if (expected_panic_string) {
		fprintf(stderr, "Tests failed - expected a panic.\n");
		return 1;
	} else {
		fprintf(stderr, "Tests passed.\n");
		return 0;
	}
}
