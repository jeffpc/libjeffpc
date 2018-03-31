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
#include <jeffpc/cbor.h>

#include "nvl_impl.h"

/*
 * nvl packing interface
 */

static int cbor_nvl_prologue(struct buffer *buffer, struct nvlist *nvl)
{
	return cbor_pack_map_start(buffer, CBOR_UNKNOWN_NELEM);
}

static int cbor_nvl_epilogue(struct buffer *buffer, struct nvlist *nvl)
{
	return cbor_pack_map_end(buffer, CBOR_UNKNOWN_NELEM);
}

static int cbor_array_prologue(struct buffer *buffer, struct val *const *vals,
			       size_t nelem)
{
	return cbor_pack_array_start(buffer, nelem);
}

static int cbor_array_epilogue(struct buffer *buffer, struct val *const *vals,
			       size_t nelem)
{
	return cbor_pack_array_end(buffer, nelem);
}

const struct nvops nvops_cbor = {
	.pack = {
		.nvl_prologue = cbor_nvl_prologue,
		.nvl_epilogue = cbor_nvl_epilogue,

		.array_prologue = cbor_array_prologue,
		.array_epilogue = cbor_array_epilogue,

		.val_blob = cbor_pack_blob,
		.val_bool = cbor_pack_bool,
		.val_int = cbor_pack_uint,
		.val_null = cbor_pack_null,
		.val_str = cbor_pack_cstr,
	}
};
