/*
 * Copyright (c) 2011-2017 Josef 'Jeff' Sipek <jeffpc@josefsipek.net>
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

#ifndef __JEFFPC_IO_COMMON_H
#define __JEFFPC_IO_COMMON_H

#include <stdbool.h>

static inline ssize_t rw(int fd, void *buf, size_t nbyte, off_t off,
			 const bool useoff, const bool readfxns,
			 size_t *resid)
{
	char *ptr = buf;
	size_t total;
	ssize_t ret;

	total = 0;

	while (nbyte) {
		if (readfxns) {
			if (useoff)
				ret = pread(fd, ptr, nbyte, off);
			else
				ret = read(fd, ptr, nbyte);
		} else {
			if (useoff)
				ret = pwrite(fd, ptr, nbyte, off);
			else
				ret = write(fd, ptr, nbyte);
		}

		if (ret < 0) {
			ret = -errno;
			goto err;
		}

		if (ret == 0) {
			ret = -EPIPE;
			goto err;
		}

		nbyte -= ret;
		total += ret;
		ptr   += ret;
		off   += ret;
	}

	ret = 0; /* success, all read/written */

err:
	if (resid)
		*resid = nbyte;

	return ret;
}

#endif
