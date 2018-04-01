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

#ifndef __JEFFPC_NVL_IMPL_H
#define __JEFFPC_NVL_IMPL_H

#include <jeffpc/nvl.h>
#include <jeffpc/buffer.h>

#define CALL(ops, op, args)						\
	({								\
		int _ret = 0;						\
									\
		if ((ops)->op)						\
			_ret = (ops)->op args;				\
									\
		_ret;							\
	 })

#define CALL_OR_FAIL(ops, op, args)					\
	({								\
		int _ret = -ENOTSUP;					\
									\
		if ((ops)->op)						\
			_ret = (ops)->op args;				\
									\
		_ret;							\
	 })

/*
 * Packing operations
 *
 * A required function pointer is only required when an item of that type
 * must be encoded.  For example, if an encoder doesn't implement packing of
 * strings, packing of all strings vals will fail because of this limitation.
 * Packing of other (non string) vals may or may not fail for
 * other reasons.
 */
struct packops {
	/*
	 * top-level encoder hooks
	 *
	 * Optional: All.
	 */
	int (*buffer_finish)(struct buffer *buffer);

	/*
	 * nvlist encoders
	 *
	 * Required: At least one of prologue or epilogue.
	 * Optional: The rest.
	 */
	int (*nvl_prologue)(struct buffer *buffer, struct nvlist *nvl);
	int (*nvl_name_sep)(struct buffer *buffer, const struct nvpair *pair);
	int (*nvl_val_sep)(struct buffer *buffer, const struct nvpair *pair);
	int (*nvl_epilogue)(struct buffer *buffer, struct nvlist *nvl);

	/*
	 * array encoders
	 *
	 * Required: At least one of prologue or epilogue.
	 * Optional: The rest.
	 */
	int (*array_prologue)(struct buffer *buffer, struct val *const *vals,
			      size_t nelem);
	int (*array_val_sep)(struct buffer *buffer, const struct val *val);
	int (*array_epilogue)(struct buffer *buffer, struct val *const *vals,
			      size_t nelem);

	/*
	 * nvpair
	 *
	 * Optional: All.
	 */
	int (*pair_prologue)(struct buffer *buffer, const struct nvpair *pair);
	int (*pair_epilogue)(struct buffer *buffer, const struct nvpair *pair);

	/*
	 * value encoders
	 *
	 * Required: All.
	 */
	int (*val_blob)(struct buffer *buffer, const void *data, size_t size);
	int (*val_bool)(struct buffer *buffer, bool b);
	int (*val_int)(struct buffer *buffer, uint64_t i);
	int (*val_null)(struct buffer *buffer);
	int (*val_str)(struct buffer *buffer, const char *str);
};

struct unpackops {
	/* TODO */
};

struct valops {
	const struct packops pack;
	const struct unpackops unpack;
};

extern const struct valops valops_cbor;
extern const struct valops valops_json;

static inline const struct valops *select_ops(enum val_format format)
{
	switch (format) {
		case VF_CBOR:
			return &valops_cbor;
		case VF_JSON:
			return &valops_json;
	}

	return NULL;
}

#endif
