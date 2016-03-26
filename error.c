/*
 * Copyright (c) 2013-2016 Josef 'Jeff' Sipek <jeffpc@josefsipek.net>
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

#include <sys/debug.h>

/*
 * clean up the pulled in defines since we want to do our own thing
 */
#undef ASSERT
#undef VERIFY
#undef ASSERT64
#undef ASSERT32
#undef IMPLY
#undef EQUIV
#undef VERIFY3S
#undef VERIFY3U
#undef VERIFY3P
#undef VERIFY0
#undef ASSERT3S
#undef ASSERT3U
#undef ASSERT3P
#undef ASSERT0

#include <sys/inttypes.h>
#include <syslog.h>
#include <stdarg.h>

#include <jeffpc/error.h>

#pragma weak jeffpc_log
void jeffpc_log(const char *fmt, ...)
{
	/*
	 * This function is a no-op but it exists to allow consumers of
	 * libjeffpc to override it with their own log implementation.
	 */
}

#pragma weak jeffpc_assfail
void jeffpc_assfail(const char *a, const char *f, int l)
{
	LOG("assertion failed: %s, file: %s, line: %d", a, f, l);

	assfail(a, f, l);

	/* this is a hack to shut up gcc */
	abort();
}

#pragma weak jeffpc_assfail3
void jeffpc_assfail3(const char *a, uintmax_t lv, const char *op, uintmax_t rv,
                     const char *f, int l)
{
	char msg[512];

	snprintf(msg, sizeof(msg), "%s (0x%"PRIx64" %s 0x%"PRIx64")", a, lv,
		 op, rv);

	LOG("assertion failed: %s, file: %s, line: %d", msg, f, l);

	assfail(msg, f, l);

	/* this is a hack to shut up gcc */
	abort();
}
