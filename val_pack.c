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

static int pack_nvl(const struct packops *ops, struct buffer *buffer,
		    struct nvlist *nvl);
static int pack_val(const struct packops *ops, struct buffer *buffer,
		    struct val *val);

static int pack_array(const struct packops *ops, struct buffer *buffer,
		      struct val **vals, size_t nelem)
{
	bool first = true;
	size_t i;
	int ret;

	if (!ops->array_prologue && !ops->array_epilogue)
		return -ENOTSUP;

	if ((ret = CALL(ops, array_prologue, (buffer, vals, nelem))))
		return ret;

	for (i = 0; i < nelem; i++) {
		struct val *val = vals[i];

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

static int pack_blob(const struct packops *ops, struct buffer *buffer,
		     const void *data, size_t size)
{
	return CALL_OR_FAIL(ops, val_blob, (buffer, data, size));
}

static int pack_bool(const struct packops *ops, struct buffer *buffer,
		     bool b)
{
	return CALL_OR_FAIL(ops, val_bool, (buffer, b));
}

static int pack_int(const struct packops *ops, struct buffer *buffer,
		    uint64_t i)
{
	return CALL_OR_FAIL(ops, val_int, (buffer, i));
}

static int pack_null(const struct packops *ops, struct buffer *buffer)
{
	return CALL_OR_FAIL(ops, val_null, (buffer));
}

static int pack_string(const struct packops *ops, struct buffer *buffer,
		       struct str *str)
{
	const char *s;

	/*
	 * Even though we try to avoid NULL pointers to indicate empty
	 * strings, they can crop up from many places.  So, it is safer to
	 * do the check here.
	 */
	s = str ? str_cstr(str) : "";

	return CALL_OR_FAIL(ops, val_str, (buffer, s));
}

static int pack_val(const struct packops *ops, struct buffer *buffer,
		    struct val *val)
{
	int ret = -ENOTSUP;

	switch (val->type) {
		case VT_ARRAY:
			ret = pack_array(ops, buffer, val->array.vals,
					 val->array.nelem);
			break;
		case VT_BLOB:
			ret = pack_blob(ops, buffer, val->blob.ptr,
					val->blob.size);
			break;
		case VT_BOOL:
			ret = pack_bool(ops, buffer, val->b);
			break;
		case VT_INT:
			ret = pack_int(ops, buffer, val->i);
			break;
		case VT_NULL:
			ret = pack_null(ops, buffer);
			break;
		case VT_NVL:
			ret = pack_nvl(ops, buffer, val_cast_to_nvl(val));
			break;
		case VT_STR:
			ret = pack_string(ops, buffer, val_cast_to_str(val));
			break;
		case VT_SYM:
		case VT_CONS:
		case VT_CHAR:
			/* not supported */
			break;
	}

	return ret;
}

static int pack_pair(const struct packops *ops, struct buffer *buffer,
		     const struct nvpair *pair)
{
	int ret;

	if ((ret = CALL(ops, pair_prologue, (buffer, pair))))
		return ret;

	if ((ret = pack_string(ops, buffer, pair->name)))
		return ret;

	if ((ret = CALL(ops, nvl_name_sep, (buffer, pair))))
		return ret;

	if ((ret = pack_val(ops, buffer, pair->value)))
		return ret;

	if ((ret = CALL(ops, pair_epilogue, (buffer, pair))))
		return ret;

	return 0;
}

static int pack_nvl(const struct packops *ops, struct buffer *buffer,
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

static int do_val_pack(struct buffer *buffer, struct val *val,
		       enum val_format format)
{
	const struct valops *ops;
	int ret;

	ops = select_ops(format);
	if (!ops)
		return -ENOTSUP;

	ret = pack_val(&ops->pack, buffer, val);
	if (ret)
		return ret;

	return CALL(&ops->pack, buffer_finish, (buffer));
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

	return ret ? ret : buffer_used(&buffer);
}
