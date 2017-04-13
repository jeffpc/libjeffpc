/*
 * Copyright (c) 2016-2017 Josef 'Jeff' Sipek <jeffpc@josefsipek.net>
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

#ifndef __JEFFPC_TIME_H
#define __JEFFPC_TIME_H

#include <jeffpc/error.h>

#include <inttypes.h>
#include <time.h>

static inline uint64_t __gettime(int clock)
{
	struct timespec ts;

	if (clock_gettime(clock, &ts) != 0)
		panic("%s: clock_gettime failed: %s", __func__,
		      strerror(errno));

	return (ts.tv_sec * 1000000000ull) + ts.tv_nsec;
}

/*
 * Get the current "time" in ns.  The epoch is arbitrary (e.g., system
 * boot).  This clock is not subject to resetting or drifting, making it
 * ideal for performance measurements.
 */
static inline uint64_t gettick(void)
{
	return __gettime(CLOCK_MONOTONIC);
}

/*
 * Get the current time in ns.  The epoch is 1970-01-01 00:00:00 UTC.
 */
static inline uint64_t gettime(void)
{
	return __gettime(CLOCK_REALTIME);
}

#endif
