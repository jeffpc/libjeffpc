/*
 * Copyright (c) 2017-2020 Josef 'Jeff' Sipek <jeffpc@josefsipek.net>
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

static int do_val_pack(struct buffer *buffer, struct val *val,
		       enum val_format format)
{
	const struct valops *ops;

	ops = select_ops(format);
	if (!ops || !ops->pack)
		return -ENOTSUP;

	return ops->pack(buffer, val);
}

ssize_t val_pack_into(struct val *val, void *buf, size_t bufsize,
		      enum val_format format)
{
	struct buffer buffer;
	int ret;

	buffer_init_static(&buffer, buf, 0, bufsize, true);

	ret = do_val_pack(&buffer, val, format);

	return ret ? ret : buffer_size(&buffer);
}

struct buffer *val_pack(struct val *val, enum val_format format)
{
	struct buffer *buffer;
	int ret;

	buffer = buffer_alloc(1024);
	if (IS_ERR(buffer))
		return buffer;

	ret = do_val_pack(buffer, val, format);
	if (!ret)
		return buffer;

	buffer_free(buffer);
	return ERR_PTR(ret);
}

ssize_t val_size(struct val *val, enum val_format format)
{
	struct buffer buffer;
	int ret;

	buffer_init_sink(&buffer);

	ret = do_val_pack(&buffer, val, format);

	return ret ? ret : buffer_size(&buffer);
}
