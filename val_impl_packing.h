/*
 * Copyright (c) 2017-2019 Josef 'Jeff' Sipek <jeffpc@josefsipek.net>
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
#include <jeffpc/cbor.h>
#include <jeffpc/json.h>

struct valops {
	int (*pack)(struct buffer *buffer, struct val *val);
	struct val *(*unpack)(struct buffer *buffer);
};

static const struct valops valops_cbor = {
	.pack = cbor_pack_val,
	.unpack = cbor_unpack_val,
};

static const struct valops valops_json = {
	.pack = json_pack_val,
};

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
