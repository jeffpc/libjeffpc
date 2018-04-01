/*
 * Copyright (c) 2017-2018 Josef 'Jeff' Sipek <jeffpc@josefsipek.net>
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

#include <jeffpc/nvl.h>
#include <jeffpc/buffer.h>

#include "val_impl_packing.h"

static int unpack_nvl(const struct nvunpackops *ops, const struct buffer *buffer,
		      struct nvlist *nvl)
{
	return -ENOTSUP; /* FIXME */
}

struct nvlist *nvl_unpack(const void *ptr, size_t len, enum nvformat format)
{
	const struct buffer buffer = {
		.data = (void *) ptr,
		.used = len,
		.allocsize = len,
	};
	const struct nvops *ops;
	struct nvlist *nvl;
	int ret;

	ops = select_ops(format);
	if (!ops)
		return ERR_PTR(-ENOTSUP);

	nvl = nvl_alloc();
	if (!nvl)
		return ERR_PTR(-ENOMEM);

	ret = unpack_nvl(&ops->unpack, &buffer, nvl);
	if (!ret)
		return nvl;

	nvl_putref(nvl);
	return ERR_PTR(ret);
}
