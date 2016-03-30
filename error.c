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

#include <inttypes.h>
#include <syslog.h>
#include <stdio.h>
#include <stdbool.h>
#include <assert.h>
#include <pthread.h>

#include <jeffpc/error.h>
#include <jeffpc/config.h>

#include "init.h"

#ifndef HAVE_ASSFAIL
static void assfail(const char *assertion, const char *file, int line)
{
	__assert(assertion, file, line);
}
#endif

void default_print(enum errlevel level, const char *fmt, va_list ap)
{
	FILE *out;

	switch (level) {
		case CE_DEBUG:
		case CE_INFO:
			out = stdout;
			break;
		case CE_WARN:
		case CE_ERROR:
		case CE_CRIT:
		case CE_PANIC:
		default:
			out = stderr;
			break;
	}

	vfprintf(out, fmt, ap);
}

void jeffpc_print(enum errlevel level, const char *fmt, ...)
{
	va_list ap;

	va_start(ap, fmt);
	libops.print(level, fmt, ap);
	va_end(ap);
}

void default_log(int loglevel, const char *fmt, va_list ap)
{
	/*
	 * This function is a no-op but it exists to allow consumers of
	 * libjeffpc to override it with their own log implementation.
	 */
}

void jeffpc_log(int loglevel, const char *fmt, ...)
{
	va_list ap;

	va_start(ap, fmt);
	libops.log(loglevel, fmt, ap);
	va_end(ap);
}

void default_assfail(const char *a, const char *f, int l)
{
	jeffpc_log(LOG_ALERT, "assertion failed: %s, file: %s, line: %d",
		   a, f, l);

	assfail(a, f, l);
}

void jeffpc_assfail(const char *a, const char *f, int l)
{
	libops.assfail(a, f, l);

	/* this is a hack to shut up gcc */
	abort();
}

void default_assfail3(const char *a, uintmax_t lv, const char *op, uintmax_t rv,
		      const char *f, int l)
{
	char msg[512];

	snprintf(msg, sizeof(msg), "%s (0x%"PRIx64" %s 0x%"PRIx64")", a, lv,
		 op, rv);

	jeffpc_log(LOG_ALERT, "assertion failed: %s, file: %s, line: %d",
		   msg, f, l);

	assfail(msg, f, l);
}

void jeffpc_assfail3(const char *a, uintmax_t lv, const char *op, uintmax_t rv,
		     const char *f, int l)
{
	libops.assfail3(a, lv, op, rv, f, l);

	/* this is a hack to shut up gcc */
	abort();
}

void cmn_verr(enum errlevel level, const char *fmt, va_list ap)
{
	const char *levelstr;
	unsigned long tid;
	int loglevel;
	bool panic;
	char buf[256];

	tid = (unsigned long) pthread_self();
	panic = false;

	switch (level) {
		case CE_DEBUG:
			levelstr = "DEBUG";
			loglevel = LOG_DEBUG;
			break;
		case CE_INFO:
			levelstr = "INFO";
			loglevel = LOG_INFO;
			break;
		case CE_WARN:
			levelstr = "WARN";
			loglevel = LOG_WARNING;
			break;
		case CE_ERROR:
			levelstr = "ERROR";
			loglevel = LOG_ERR;
			break;
		case CE_CRIT:
			levelstr = "CRIT";
			loglevel = LOG_CRIT;
			break;
		case CE_PANIC:
			levelstr = "PANIC";
			loglevel = LOG_ALERT;
			panic = true;
			break;
		default:
			levelstr = "?????";
			loglevel = LOG_CRIT;
			panic = true;
			break;
	}

	vsnprintf(buf, sizeof(buf), fmt, ap);

	/*
	 * We are printing the thread ID as a 4-digit number. This will
	 * allow systems that use small integers (e.g., Illumos) to have
	 * short IDs.  Systems that use large integers (e.g., Linux) will
	 * use more digits.  Since on those systems the IDs will be
	 * clustered around some big integer, they will very likely always
	 * print as the same number of digits.
	 */
	jeffpc_log(loglevel, "[%04lx] %-5s %s\n", tid, levelstr, buf);
	jeffpc_print(level, "[%04lx] %-5s %s\n", tid, levelstr, buf);

	if (panic)
		abort();
}

void cmn_err(enum errlevel level, const char *fmt, ...)
{
	va_list ap;

	va_start(ap, fmt);
	cmn_verr(level, fmt, ap);
	va_end(ap);
}

void panic(const char *fmt, ...)
{
	va_list ap;

	va_start(ap, fmt);
	cmn_verr(CE_PANIC, fmt, ap);
	va_end(ap);

	/* this is a hack to shut up gcc */
	abort();
}
