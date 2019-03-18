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

#ifndef __JEFFPC_CBOR_H
#define __JEFFPC_CBOR_H

#include <jeffpc/buffer.h>
#include <jeffpc/val.h>

#define CBOR_UNKNOWN_NELEM	(~0ul)

/*
 * On failure, the buffer may contain partially encoded data items.  On
 * success, a fully encoded data item is appended to the buffer.
 */
extern int cbor_pack_uint(struct buffer *buffer, uint64_t v);
extern int cbor_pack_nint(struct buffer *buffer, uint64_t v);
extern int cbor_pack_int(struct buffer *buffer, int64_t v);
extern int cbor_pack_blob(struct buffer *buffer, const void *data,
			  size_t size);
extern int cbor_pack_cstr_len(struct buffer *buffer, const char *str, size_t len);
extern int cbor_pack_str(struct buffer *buffer, struct str *str);
extern int cbor_pack_bool(struct buffer *buffer, bool b);
extern int cbor_pack_null(struct buffer *buffer);
extern int cbor_pack_break(struct buffer *buffer);
extern int cbor_pack_array_start(struct buffer *buffer, size_t nelem);
extern int cbor_pack_array_end(struct buffer *buffer, size_t nelem);
extern int cbor_pack_array_vals(struct buffer *buffer, struct val **vals,
				size_t nelem);
extern int cbor_pack_map_start(struct buffer *buffer, size_t npairs);
extern int cbor_pack_map_end(struct buffer *buffer, size_t npairs);
extern int cbor_pack_val(struct buffer *buffer, struct val *val);
extern int cbor_pack_map_val(struct buffer *buffer, struct val *val);

static inline int cbor_pack_cstr(struct buffer *buffer, const char *str)
{
	return cbor_pack_cstr_len(buffer, str, strlen(str));
}

extern int cbor_peek_type(struct buffer *buffer, enum val_type *type);

/*
 * On failure, the buffer state is unchanged.  On success, the buffer is
 * updated to point to the first byte of the next data item.
 */
extern int cbor_unpack_uint(struct buffer *buffer, uint64_t *v);
extern int cbor_unpack_nint(struct buffer *buffer, uint64_t *v);
extern int cbor_unpack_int(struct buffer *buffer, int64_t *v);
extern int cbor_unpack_blob(struct buffer *buffer, void **data, size_t *size);
extern int cbor_unpack_cstr_len(struct buffer *buffer, char **str,
				size_t *len);
extern int cbor_unpack_str(struct buffer *buffer, struct str **str);
extern int cbor_unpack_bool(struct buffer *buffer, bool *b);
extern int cbor_unpack_null(struct buffer *buffer);
extern int cbor_unpack_break(struct buffer *buffer);
extern int cbor_unpack_map_start(struct buffer *buffer, uint64_t *npairs,
				 bool *end_required);
extern int cbor_unpack_map_end(struct buffer *buffer, bool end_required);
extern int cbor_unpack_array_start(struct buffer *buffer, uint64_t *nelem,
				   bool *end_required);
extern int cbor_unpack_array_end(struct buffer *buffer, bool end_required);
extern struct val *cbor_unpack_val(struct buffer *buffer);

#endif
