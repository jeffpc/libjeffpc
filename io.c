/*
 * Copyright (c) 2011-2016 Josef 'Jeff' Sipek <jeffpc@josefsipek.net>
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

#include <unistd.h>
#include <errno.h>
#include <stdlib.h>
#include <stdio.h>
#include <errno.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <unistd.h>
#include <fcntl.h>
#include <string.h>

#include <jeffpc/io.h>

int xread(int fd, void *buf, size_t nbyte)
{
	char *ptr = buf;
	size_t total;
	int ret;

	total = 0;

	while (nbyte) {
		ret = read(fd, ptr, nbyte);
		if (ret < 0) {
			LOG("%s: failed to read %u bytes from fd %d: %s",
			    __func__, nbyte, fd, strerror(errno));
			return errno;
		}

		if (ret == 0)
			return EPIPE;

		nbyte -= ret;
		total += ret;
		ptr   += ret;
	}

	return 0;
}

int xwrite(int fd, const void *buf, size_t nbyte)
{
	const char *ptr = buf;
	size_t total;
	int ret;

	total = 0;

	while (nbyte) {
		ret = write(fd, ptr, nbyte);
		if (ret < 0) {
			LOG("%s: failed to write %u bytes to fd %d: %s",
			    __func__, nbyte, fd, strerror(errno));
			return errno;
		}

		if (ret == 0)
			return EPIPE;

		nbyte -= ret;
		total += ret;
		ptr   += ret;
	}

	return 0;
}

char *read_file_common(const char *fname, struct stat *sb)
{
	struct stat statbuf;
	char *out;
	int ret;
	int fd;

	fd = open(fname, O_RDONLY);
	if (fd == -1) {
		out = ERR_PTR(errno);
		goto err;
	}

	ret = fstat(fd, &statbuf);
	if (ret == -1) {
		out = ERR_PTR(errno);
		goto err_close;
	}

	out = malloc(statbuf.st_size + 1);
	if (!out) {
		out = ERR_PTR(ENOMEM);
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
	close(fd);

err:
	return out;
}

int write_file(const char *fname, const char *data, size_t len)
{
	int ret;
	int fd;

	fd = open(fname, O_WRONLY | O_CREAT | O_EXCL,
		  S_IRUSR | S_IWUSR | S_IRGRP | S_IROTH);
	if (fd == -1)
		return errno;

	ret = xwrite(fd, data, len);

	close(fd);

	return ret;
}