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

#include <jeffpc/cbor.h>
#include <jeffpc/nvl.h>

enum major_type {
	CMT_UINT  = 0,
	CMT_NINT  = 1,
	CMT_BYTE  = 2,
	CMT_TEXT  = 3,
	CMT_ARRAY = 4,
	CMT_MAP	  = 5,
	CMT_TAG   = 6,
	CMT_FLOAT = 7,
};

#define ADDL_UINT8		24
#define ADDL_UINT16		25
#define ADDL_UINT32		26
#define ADDL_UINT64		27
#define ADDL_ARRAY_INDEF	31
#define ADDL_MAP_INDEF		31
#define ADDL_FLOAT_FALSE	20
#define ADDL_FLOAT_TRUE		21
#define ADDL_FLOAT_NULL		22
#define ADDL_FLOAT_BREAK	31

#define MKTYPE(type, additional)					\
		({							\
			ASSERT0(type & ~0x7);				\
			ASSERT0(additional & ~0x1f);			\
			(type << 5) | additional;			\
		})

#define MKTYPE_STATIC(type, additional)					\
		({							\
			STATIC_ASSERT0(type & ~0x7);			\
			STATIC_ASSERT0(additional & ~0x1f);		\
			(type << 5) | additional;			\
		})

static int pack_cbor_type_byte(struct buffer *buffer, enum major_type type,
			       uint8_t additional)
{
	uint8_t byte;

	byte = MKTYPE(type, additional);

	return buffer_append(buffer, &byte, 1);
}

/* pack a type byte with specified additional information */
static inline int pack_cbor_type(struct buffer *buffer, enum major_type type,
				 uint64_t additional)
{
	union {
		uint64_t u64;
		uint32_t u32;
		uint16_t u16;
		uint8_t u8;
	} u;
	uint8_t addl;
	size_t size;
	int ret;

	if (additional <= 23)
		return pack_cbor_type_byte(buffer, type, additional);

	if (additional <= 0xff) {
		size = sizeof(uint8_t);
		addl = ADDL_UINT8;
		u.u8 = cpu8_to_be(additional);
	} else if (additional <= 0xffff) {
		size = sizeof(uint16_t);
		addl = ADDL_UINT16;
		u.u16 = cpu16_to_be(additional);
	} else if (additional <= 0xffffffff) {
		size = sizeof(uint32_t);
		addl = ADDL_UINT32;
		u.u32 = cpu32_to_be(additional);
	} else {
		size = sizeof(uint64_t);
		addl = ADDL_UINT64;
		u.u64 = cpu64_to_be(additional);
	}

	if ((ret = pack_cbor_type_byte(buffer, type, addl)))
		return ret;

	return buffer_append(buffer, &u, size);
}

int cbor_pack_uint(struct buffer *buffer, uint64_t v)
{
	return pack_cbor_type(buffer, CMT_UINT, v);
}

int cbor_pack_nint(struct buffer *buffer, uint64_t v)
{
	return -ENOTSUP; /* TODO */
}

int cbor_pack_int(struct buffer *buffer, int64_t v)
{
	if (v < 0)
		return cbor_pack_nint(buffer, -v);

	return cbor_pack_uint(buffer, v);
}

int cbor_pack_blob(struct buffer *buffer, const void *data,
		   size_t size)
{
	int ret;

	if ((ret = pack_cbor_type(buffer, CMT_BYTE, size)))
		return ret;

	return buffer_append(buffer, data, size);
}

int cbor_pack_cstr_len(struct buffer *buffer, const char *str, size_t len)
{
	int ret;

	if ((ret = pack_cbor_type(buffer, CMT_TEXT, len)))
		return ret;

	return buffer_append(buffer, str, len);
}

int cbor_pack_str(struct buffer *buffer, struct str *str)
{
	return cbor_pack_cstr_len(buffer, str_cstr(str), str_len(str));
}

int cbor_pack_bool(struct buffer *buffer, bool b)
{
	/* bools use the float major type */
	return pack_cbor_type(buffer, CMT_FLOAT,
			      b ? ADDL_FLOAT_TRUE : ADDL_FLOAT_FALSE);
}

int cbor_pack_null(struct buffer *buffer)
{
	/* null uses the float major type */
	return pack_cbor_type(buffer, CMT_FLOAT, ADDL_FLOAT_NULL);
}

int cbor_pack_break(struct buffer *buffer)
{
	static const uint8_t byte = MKTYPE_STATIC(CMT_FLOAT, ADDL_FLOAT_BREAK);

	return buffer_append(buffer, &byte, 1);
}

int cbor_pack_array_start(struct buffer *buffer, size_t nelem)
{
	static const uint8_t byte = MKTYPE_STATIC(CMT_ARRAY, ADDL_ARRAY_INDEF);

	if (nelem != CBOR_UNKNOWN_NELEM)
		return pack_cbor_type(buffer, CMT_ARRAY, nelem);

	/* indefinite-length array */
	return buffer_append(buffer, &byte, 1);
}

int cbor_pack_array_end(struct buffer *buffer, size_t nelem)
{
	if (nelem != CBOR_UNKNOWN_NELEM)
		return 0;

	/* indefinite-length array */
	return cbor_pack_break(buffer);
}

int cbor_pack_array_vals(struct buffer *buffer, struct val **vals, size_t nelem)
{
	size_t i;
	int ret;

	ret = cbor_pack_array_start(buffer, nelem);
	if (ret)
		return ret;

	for (i = 0; i < nelem; i++) {
		ret = cbor_pack_val(buffer, vals[i]);
		if (ret)
			return ret;
	}

	return cbor_pack_array_end(buffer, nelem);
}

int cbor_pack_map_start(struct buffer *buffer, size_t npairs)
{
	static const uint8_t byte = MKTYPE_STATIC(CMT_MAP, ADDL_MAP_INDEF);

	if (npairs != CBOR_UNKNOWN_NELEM)
		return -ENOTSUP; /* FIXME */

	/* indefinite-length map */
	return buffer_append(buffer, &byte, 1);
}

int cbor_pack_map_end(struct buffer *buffer, size_t npairs)
{
	if (npairs != CBOR_UNKNOWN_NELEM)
		return 0;

	/* indefinite-length map */
	return cbor_pack_break(buffer);
}

int cbor_pack_map_val(struct buffer *buffer, struct val *val)
{
	struct rb_tree *tree = &val->_set_nvl.values;
	struct nvpair *cur;
	size_t npairs;
	int ret;

	if (val->type != VT_NVL)
		return -EINVAL;

	npairs = rb_numnodes(tree);

	ret = cbor_pack_map_start(buffer, npairs);
	if (ret)
		return ret;

	rb_for_each(tree, cur) {
		ret = cbor_pack_str(buffer, cur->name);
		if (ret)
			return ret;

		ret = cbor_pack_val(buffer, cur->value);
		if (ret)
			return ret;
	}

	return cbor_pack_map_end(buffer, npairs);
}

int cbor_pack_val(struct buffer *buffer, struct val *val)
{
	switch (val->type) {
		case VT_NULL:
			return cbor_pack_null(buffer);
		case VT_INT:
			return cbor_pack_uint(buffer, val->i);
		case VT_STR:
			return cbor_pack_str(buffer, val_cast_to_str(val));
		case VT_SYM:
			return -ENOTSUP;
		case VT_BOOL:
			return cbor_pack_bool(buffer, val->b);
		case VT_CONS:
			return -ENOTSUP;
		case VT_CHAR:
			return -ENOTSUP;
		case VT_BLOB:
			return cbor_pack_blob(buffer, val->blob.ptr,
					      val->blob.size);
		case VT_ARRAY:
			/*
			 * We need to pass non-const struct vals so that we
			 * can use val_cast_to_*().
			 */
			return cbor_pack_array_vals(buffer, val->_set_array.vals,
						    val->array.nelem);
		case VT_NVL:
			return cbor_pack_map_val(buffer, val);
	}

	return -ENOTSUP;
}
