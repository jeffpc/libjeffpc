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

#include <jeffpc/thread.h>
#include <jeffpc/error.h>
#include <jeffpc/mem.h>

struct xthr_info {
	void *(*f)(void *);
	void *arg;
};

static void *xthr_setup(void *_info)
{
	struct xthr_info info = *((struct xthr_info *) _info);

	/* free early since the function may run for a very long time */
	free(_info);

	return info.f(info.arg);
}

int xthr_create(pthread_t *restrict thread, void *(*start)(void*),
		void *restrict arg)
{
	struct xthr_info *info;
	pthread_t tmp;
	int ret;

	if (!start)
		return -EINVAL;

	info = malloc(sizeof(struct xthr_info));
	if (!info)
		return -ENOMEM;

	info->f = start;
	info->arg = arg;

	ret = -pthread_create(thread ? thread : &tmp, NULL, xthr_setup, info);
	if (ret)
		free(info);

	return ret;
}
