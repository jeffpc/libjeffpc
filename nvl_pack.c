/*
 * Copyright (c) 2017 Josef 'Jeff' Sipek <jeffpc@josefsipek.net>
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

#include "nvl_impl.h"

static int pack_nvl(const struct nvpackops *ops, struct buffer *buffer,
		    struct nvlist *nvl);
static int pack_val(const struct nvpackops *ops, struct buffer *buffer,
		    const struct nvval *val);

static int pack_array(const struct nvpackops *ops, struct buffer *buffer,
		      const struct nvval *vals, size_t nelem)
{
	bool first = true;
	size_t i;
	int ret;

	if (!ops->array_prologue && !ops->array_epilogue)
		return -ENOTSUP;

	if ((ret = CALL(ops, array_prologue, (buffer, vals, nelem))))
		return ret;

	for (i = 0; i < nelem; i++) {
		const struct nvval *val = &vals[i];

		if (first) {
			first = false;
			if ((ret = CALL(ops, array_val_sep, (buffer, val))))
				return ret;
		}

		if ((ret = pack_val(ops, buffer, val)))
			return ret;
	}

	if ((ret = CALL(ops, array_epilogue, (buffer, vals, nelem))))
		return ret;

	return 0;
}

static int pack_blob(const struct nvpackops *ops, struct buffer *buffer,
		     const void *data, size_t size)
{
	return CALL_OR_FAIL(ops, val_blob, (buffer, data, size));
}

static int pack_bool(const struct nvpackops *ops, struct buffer *buffer,
		     bool b)
{
	return CALL_OR_FAIL(ops, val_bool, (buffer, b));
}

static int pack_int(const struct nvpackops *ops, struct buffer *buffer,
		    uint64_t i)
{
	return CALL_OR_FAIL(ops, val_int, (buffer, i));
}

static int pack_null(const struct nvpackops *ops, struct buffer *buffer)
{
	return CALL_OR_FAIL(ops, val_null, (buffer));
}

static int pack_cstring(const struct nvpackops *ops, struct buffer *buffer,
			const char *str)
{
	return CALL_OR_FAIL(ops, val_str, (buffer, str));
}

static int pack_string(const struct nvpackops *ops, struct buffer *buffer,
		       struct str *str)
{
	return pack_cstring(ops, buffer, str_cstr(str));
}

static int pack_val(const struct nvpackops *ops, struct buffer *buffer,
		    const struct nvval *val)
{
	int ret = -ENOTSUP;

	switch (val->type) {
		case NVT_ARRAY:
			ret = pack_array(ops, buffer, val->array.vals,
					 val->array.nelem);
			break;
		case NVT_BLOB:
			ret = pack_blob(ops, buffer, val->blob.ptr,
					val->blob.size);
			break;
		case NVT_BOOL:
			ret = pack_bool(ops, buffer, val->b);
			break;
		case NVT_INT:
			ret = pack_int(ops, buffer, val->i);
			break;
		case NVT_NULL:
			ret = pack_null(ops, buffer);
			break;
		case NVT_NVL:
			ret = pack_nvl(ops, buffer, val->nvl);
			break;
		case NVT_STR:
			ret = pack_string(ops, buffer, val->str);
			break;
	}

	return ret;
}

static int pack_pair(const struct nvpackops *ops, struct buffer *buffer,
		     const struct nvpair *pair)
{
	int ret;

	if ((ret = CALL(ops, pair_prologue, (buffer, pair))))
		return ret;

	if ((ret = pack_cstring(ops, buffer, pair->name)))
		return ret;

	if ((ret = CALL(ops, nvl_name_sep, (buffer, pair))))
		return ret;

	if ((ret = pack_val(ops, buffer, &pair->value)))
		return ret;

	if ((ret = CALL(ops, pair_epilogue, (buffer, pair))))
		return ret;

	return 0;
}

static int pack_nvl(const struct nvpackops *ops, struct buffer *buffer,
		    struct nvlist *nvl)
{
	const struct nvpair *pair;
	bool first = true;
	int ret;

	if (!ops->nvl_prologue && !ops->nvl_epilogue)
		return -ENOTSUP;

	if ((ret = CALL(ops, nvl_prologue, (buffer, nvl))))
		return ret;

	nvl_for_each(pair, nvl) {
		if (first) {
			first = false;
		} else {
			if ((ret = CALL(ops, nvl_val_sep, (buffer, pair))))
				return ret;
		}

		if ((ret = pack_pair(ops, buffer, pair)))
			return ret;
	}

	if ((ret = CALL(ops, nvl_epilogue, (buffer, nvl))))
		return ret;

	return 0;
}

static int __nvl_pack(const struct nvpackops *ops, struct buffer *buffer,
		      struct nvlist *nvl, bool call_finish)
{
	int ret;

	ret = pack_nvl(ops, buffer, nvl);
	if (ret)
		return ret;

	if (!call_finish)
		return 0;

	return CALL(ops, buffer_finish, (buffer));
}

struct buffer *nvl_pack(struct nvlist *nvl, enum nvformat format)
{
	const struct nvops *ops;
	struct buffer *buffer;
	int ret;

	ops = select_ops(format);
	if (!ops)
		return ERR_PTR(-ENOTSUP);

	buffer = buffer_alloc(1024);
	if (IS_ERR(buffer))
		return buffer;

	ret = __nvl_pack(&ops->pack, buffer, nvl, true);
	if (!ret)
		return buffer;

	buffer_free(buffer);
	return ERR_PTR(ret);
}

ssize_t nvl_size(struct nvlist *nvl, enum nvformat format)
{
	const struct nvops *ops;
	struct buffer buffer;
	int ret;

	ops = select_ops(format);
	if (!ops)
		return -ENOTSUP;

	buffer_init_sink(&buffer);

	ret = __nvl_pack(&ops->pack, &buffer, nvl, false);

	return ret ? ret : buffer_used(&buffer);
}