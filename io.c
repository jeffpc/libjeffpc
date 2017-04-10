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

#include <jeffpc/io.h>
#include <jeffpc/error.h>

#include "io_common.h"

int xread(int fd, void *buf, size_t nbyte)
{
	return rw(fd, buf, nbyte, 0, false, true, NULL);
}

int xpread(int fd, void *buf, size_t nbyte, off_t off)
{
	return rw(fd, buf, nbyte, off, true, true, NULL);
}

int xwrite(int fd, const void *buf, size_t nbyte)
{
	return rw(fd, (void *) buf, nbyte, 0, false, false, NULL);
}

int xpwrite(int fd, const void *buf, size_t nbyte, off_t off)
{
	return rw(fd, (void *) buf, nbyte, off, true, false, NULL);
}

char *read_file_common(int dirfd, const char *fname, struct stat *sb)
{
	struct stat statbuf;
	char *out;
	int ret;
	int fd;

	fd = xopenat(dirfd, fname, O_RDONLY, 0);
	if (fd < 0) {
		out = ERR_PTR(fd);
		goto err;
	}

	ret = fstat(fd, &statbuf);
	if (ret == -1) {
		out = ERR_PTR(-errno);
		goto err_close;
	}

	out = malloc(statbuf.st_size + 1);
	if (!out) {
		out = ERR_PTR(-ENOMEM);
		goto err_close;
	}

	ret = xread(fd, out, statbuf.st_size);
	if (ret) {
		free(out);
		out = ERR_PTR(ret);
	} else {
		out[statbuf.st_size] = '\0';

		if (sb)
			*sb = statbuf;
	}

err_close:
	xclose(fd);

err:
	return out;
}

int write_file(const char *fname, const char *data, size_t len)
{
	int ret;
	int fd;

	fd = xopen(fname, O_WRONLY | O_CREAT | O_EXCL,
		   S_IRUSR | S_IWUSR | S_IRGRP | S_IROTH);
	if (fd < 0)
		return fd;

	ret = xwrite(fd, data, len);

	xclose(fd);

	return ret;
}
