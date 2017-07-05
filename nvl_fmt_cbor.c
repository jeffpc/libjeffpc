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

#include "nvl_impl.h"

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

static inline int pack_cbor_break(struct buffer *buffer)
{
	static const uint8_t byte = MKTYPE_STATIC(CMT_FLOAT, ADDL_FLOAT_BREAK);

	return buffer_append(buffer, &byte, 1);
}

static inline int pack_cbor_uint(struct buffer *buffer, uint64_t i)
{
	return pack_cbor_type(buffer, CMT_UINT, i);
}

/* not needed since we don't have negative ints: pack_cbor_nint */

static inline int pack_cbor_byte(struct buffer *buffer, const void *data,
				 size_t size)
{
	int ret;

	if ((ret = pack_cbor_type(buffer, CMT_BYTE, size)))
		return ret;

	return buffer_append(buffer, data, size);
}

static inline int pack_cbor_text(struct buffer *buffer, const char *str,
				 size_t len)
{
	int ret;

	if ((ret = pack_cbor_type(buffer, CMT_TEXT, len)))
		return ret;

	return buffer_append(buffer, str, len);
}

static inline int pack_cbor_array_start(struct buffer *buffer, size_t nelem)
{
	static const uint8_t byte = MKTYPE_STATIC(CMT_ARRAY, ADDL_ARRAY_INDEF);

	if (nelem != ~0ul)
		return pack_cbor_type(buffer, CMT_ARRAY, nelem);

	/* indefinite-length array */
	return buffer_append(buffer, &byte, 1);
}

static inline int pack_cbor_array_end(struct buffer *buffer, size_t nelem)
{
	if (nelem != ~0ul)
		return 0;

	/* indefinite-length array */
	return pack_cbor_break(buffer);
}

static inline int pack_cbor_map_start(struct buffer *buffer, size_t npairs)
{
	static const uint8_t byte = MKTYPE_STATIC(CMT_MAP, ADDL_MAP_INDEF);

	if (npairs != ~0ul)
		return -ENOTSUP; /* FIXME */

	/* indefinite-length map */
	return buffer_append(buffer, &byte, 1);
}

static inline int pack_cbor_map_end(struct buffer *buffer, size_t npairs)
{
	if (npairs != ~0ul)
		return 0;

	/* indefinite-length map */
	return pack_cbor_break(buffer);
}

static inline int pack_cbor_bool(struct buffer *buffer, bool b)
{
	/* bools use the float major type */
	return pack_cbor_type(buffer, CMT_FLOAT,
			      b ? ADDL_FLOAT_TRUE : ADDL_FLOAT_FALSE);
}

static inline int pack_cbor_null(struct buffer *buffer)
{
	/* null uses the float major type */
	return pack_cbor_type(buffer, CMT_FLOAT, ADDL_FLOAT_NULL);
}

/*
 * nvl packing interface
 */

static int cbor_nvl_prologue(struct buffer *buffer, struct nvlist *nvl)
{
	return pack_cbor_map_start(buffer, ~0ul);
}

static int cbor_nvl_epilogue(struct buffer *buffer, struct nvlist *nvl)
{
	return pack_cbor_map_end(buffer, ~0ul);
}

static int cbor_array_prologue(struct buffer *buffer, const struct nvval *vals,
			       size_t nelem)
{
	return pack_cbor_array_start(buffer, nelem);
}

static int cbor_array_epilogue(struct buffer *buffer, const struct nvval *vals,
			       size_t nelem)
{
	return pack_cbor_array_end(buffer, nelem);
}

static int cbor_val_blob(struct buffer *buffer, const void *data, size_t size)
{
	return pack_cbor_byte(buffer, data, size);
}

static int cbor_val_bool(struct buffer *buffer, bool b)
{
	return pack_cbor_bool(buffer, b);
}

static int cbor_val_int(struct buffer *buffer, uint64_t i)
{
	return pack_cbor_uint(buffer, i);
}

static int cbor_val_null(struct buffer *buffer)
{
	return pack_cbor_null(buffer);
}

static int cbor_val_str(struct buffer *buffer, const char *str)
{
	return pack_cbor_text(buffer, str, strlen(str));
}

const struct nvops nvops_cbor = {
	.pack = {
		.nvl_prologue = cbor_nvl_prologue,
		.nvl_epilogue = cbor_nvl_epilogue,

		.array_prologue = cbor_array_prologue,
		.array_epilogue = cbor_array_epilogue,

		.val_blob = cbor_val_blob,
		.val_bool = cbor_val_bool,
		.val_int = cbor_val_int,
		.val_null = cbor_val_null,
		.val_str = cbor_val_str,
	}
};
