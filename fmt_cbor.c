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

#include <jeffpc/mem.h>
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

/*
 * pack
 */

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
	if (npairs == CBOR_UNKNOWN_NELEM) {
		/* indefinite-length map */
		static const uint8_t byte = MKTYPE_STATIC(CMT_MAP,
							  ADDL_MAP_INDEF);

		return buffer_append(buffer, &byte, 1);
	} else {
		/* definite-length map */
		return pack_cbor_type(buffer, CMT_MAP, npairs);
	}
}

int cbor_pack_map_end(struct buffer *buffer, size_t npairs)
{
	if (npairs == CBOR_UNKNOWN_NELEM) {
		/* indefinite-length map */
		return cbor_pack_break(buffer);
	} else {
		/* definite-length map */
		return 0;
	}
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

/*
 * peek
 */

int cbor_peek_type(struct buffer *buffer, enum val_type *type)
{
	const uint8_t *ptr;
	enum major_type major_type;
	uint8_t extra;

	if (buffer_remain(buffer) < 1)
		return -EFAULT;

	ptr = buffer_data_current(buffer);

	major_type = *ptr >> 5;
	extra = *ptr & 0x1f;

	switch (major_type) {
		case CMT_UINT:
			*type = VT_INT;
			break;
		case CMT_BYTE:
			*type = VT_BLOB;
			break;
		case CMT_TEXT:
			*type = VT_STR;
			break;
		case CMT_FLOAT:
			switch (extra) {
				case ADDL_FLOAT_FALSE:
				case ADDL_FLOAT_TRUE:
					*type = VT_BOOL;
					break;
				case ADDL_FLOAT_NULL:
					*type = VT_NULL;
					break;
				case ADDL_FLOAT_BREAK:
					return -EINTR;
				default:
					return -ENOTSUP;
			}
			break;
		case CMT_ARRAY:
			*type = VT_ARRAY;
			break;
		case CMT_MAP:
			*type = VT_NVL;
			break;
		case CMT_NINT:
		case CMT_TAG:
			return -ENOTSUP;
	}

	return 0;
}

/*
 * unpack
 */

static struct val *unpack_cbor_val(struct buffer *buffer);

static int read_cbor_type(struct buffer *buffer, enum major_type *type,
			  uint8_t *extra)
{
	uint8_t byte;
	ssize_t ret;

	ret = buffer_read(buffer, &byte, 1);
	if (ret < 1)
		return ret ? ret : -EINVAL;

	*type = byte >> 5;
	*extra = byte & 0x1f;

	return 0;
}

static int get_addl_bytes(struct buffer *buffer, uint8_t extra, uint64_t *out)
{
	const void *ptr;
	ssize_t ret;
	size_t size;

	switch (extra) {
		case ADDL_UINT8:
			size = 1;
			break;
		case ADDL_UINT16:
			size = 2;
			break;
		case ADDL_UINT32:
			size = 4;
			break;
		case ADDL_UINT64:
			size = 8;
			break;
		default:
			if (extra > 23)
				return -EINVAL;

			size = 0;
			break;
	}

	if (buffer_remain(buffer) < size)
		return -EINVAL;

	ptr = buffer_data_current(buffer);

	switch (size) {
		case 0:
			*out = extra;
			break;
		case 1:
			*out = be8_to_cpu_unaligned(ptr);
			break;
		case 2:
			*out = be16_to_cpu_unaligned(ptr);
			break;
		case 4:
			*out = be32_to_cpu_unaligned(ptr);
			break;
		case 8:
			*out = be64_to_cpu_unaligned(ptr);
			break;
	}

	ret = buffer_seek(buffer, size, SEEK_CUR);

	return (ret < 0) ? ret : 0;
}

static int unpack_cbor_int(struct buffer *buffer, enum major_type expected_type,
			   uint64_t *out)
{
	enum major_type type;
	uint8_t extra;
	int ret;

	ret = read_cbor_type(buffer, &type, &extra);
	if (ret)
		return ret;

	if (expected_type != type)
		return -EILSEQ;

	return get_addl_bytes(buffer, extra, out);
}

/* NOTE: the FLOAT major type is used for a *lot* of different things */
static int unpack_cbor_float(struct buffer *buffer, uint8_t *extra)
{
	enum major_type type;
	int ret;

	ret = read_cbor_type(buffer, &type, extra);
	if (ret)
		return ret;

	if (type != CMT_FLOAT)
		return -EILSEQ;

	switch (*extra) {
		case ADDL_FLOAT_FALSE:
		case ADDL_FLOAT_TRUE:
		case ADDL_FLOAT_NULL:
		case ADDL_FLOAT_BREAK:
			return 0;
	}

	return -EILSEQ;
}

static int unpack_cbor_arraymap_start(struct buffer *buffer,
				      enum major_type exp_type, uint8_t indef,
				      uint64_t *len, bool *end_required)
{
	enum major_type type;
	uint8_t extra;
	int ret;

	ret = read_cbor_type(buffer, &type, &extra);
	if (ret)
		return ret;

	if (type != exp_type)
		return -EILSEQ;

	if (extra == indef) {
		*end_required = true;
		*len = 0;
		return 0;
	} else {
		*end_required = false;
		return get_addl_bytes(buffer, extra, len);
	}
}

static int sync_buffers(struct buffer *orig, struct buffer *tmp)
{
	ssize_t ret;

	ret = buffer_seek(orig, buffer_offset(tmp), SEEK_CUR);

	return (ret < 0) ? ret : 0;
}

int cbor_unpack_uint(struct buffer *buffer, uint64_t *v)
{
	struct buffer tmp;
	int ret;

	buffer_init_static(&tmp, buffer_data_current(buffer),
			   buffer_remain(buffer), false);

	ret = unpack_cbor_int(&tmp, CMT_UINT, v);
	if (ret)
		return ret;

	return sync_buffers(buffer, &tmp);
}

int cbor_unpack_nint(struct buffer *buffer, uint64_t *v)
{
	struct buffer tmp;
	int ret;

	buffer_init_static(&tmp, buffer_data_current(buffer),
			   buffer_remain(buffer), false);

	ret = unpack_cbor_int(&tmp, CMT_NINT, v);
	if (ret)
		return ret;

	return sync_buffers(buffer, &tmp);
}

int cbor_unpack_int(struct buffer *buffer, int64_t *v)
{
	struct buffer tmp;
	uint64_t tmpv;
	int ret;

	/*
	 * First, try unsigned ints
	 */

	buffer_init_static(&tmp, buffer_data_current(buffer),
			   buffer_remain(buffer), false);

	ret = cbor_unpack_uint(&tmp, &tmpv);
	if (!ret) {
		if (tmpv > INT64_MAX)
			return -EOVERFLOW;

		*v = tmpv;

		return sync_buffers(buffer, &tmp);
	}

	/*
	 * Second, try negative ints
	 */

	buffer_init_static(&tmp, buffer_data_current(buffer),
			   buffer_remain(buffer), false);

	ret = cbor_unpack_nint(&tmp, &tmpv);
	if (!ret) {
		/* 2's complement has one extra negative number */
		if (tmpv > (((uint64_t) INT64_MAX) + 1))
			return -EOVERFLOW;

		*v = ((~tmpv) + 1);

		return sync_buffers(buffer, &tmp);
	}

	return ret;
}

int cbor_unpack_blob(struct buffer *buffer, void **data, size_t *size)
{
	return -ENOTSUP;
}

int cbor_unpack_cstr_len(struct buffer *buffer, char **str, size_t *len)
{
	uint64_t parsed_len;
	struct buffer tmp;
	ssize_t ret;
	char *out;

	buffer_init_static(&tmp, buffer_data_current(buffer),
			   buffer_remain(buffer), false);

	ret = unpack_cbor_int(&tmp, CMT_TEXT, &parsed_len);
	if (ret)
		return ret;

	/* can't handle strings longer than what fits in memory */
	if (parsed_len > SIZE_MAX)
		return -EOVERFLOW;

	out = malloc(parsed_len + 1);
	if (!out)
		return -ENOMEM;

	ret = buffer_read(&tmp, out, parsed_len);
	if (ret < 0)
		goto err;

	/* must read exactly the number of bytes */
	if (ret != parsed_len) {
		ret = -EILSEQ;
		goto err;
	}

	out[parsed_len] = '\0';

	*len = parsed_len;
	*str = out;

	return sync_buffers(buffer, &tmp);

err:
	free(out);

	return ret;
}

int cbor_unpack_str(struct buffer *buffer, struct str **str)
{
	char *s;
	size_t len;
	int ret;

	ret = cbor_unpack_cstr_len(buffer, &s, &len);
	if (ret)
		return ret;

	*str = str_alloc(s);

	return IS_ERR(*str) ? PTR_ERR(*str) : 0;
}

int cbor_unpack_bool(struct buffer *buffer, bool *b)
{
	struct buffer tmp;
	uint8_t extra;
	int ret;

	buffer_init_static(&tmp, buffer_data_current(buffer),
			   buffer_remain(buffer), false);

	ret = unpack_cbor_float(&tmp, &extra);
	if (ret)
		return ret;

	switch (extra) {
		case ADDL_FLOAT_FALSE:
			*b = false;
			break;
		case ADDL_FLOAT_TRUE:
			*b = true;
			break;
		default:
			return -EILSEQ;
	}

	return sync_buffers(buffer, &tmp);
}

int cbor_unpack_null(struct buffer *buffer)
{
	struct buffer tmp;
	uint8_t extra;
	int ret;

	buffer_init_static(&tmp, buffer_data_current(buffer),
			   buffer_remain(buffer), false);

	ret = unpack_cbor_float(&tmp, &extra);
	if (ret)
		return ret;

	switch (extra) {
		case ADDL_FLOAT_NULL:
			break;
		default:
			return -EILSEQ;
	}

	return sync_buffers(buffer, &tmp);
}

int cbor_unpack_break(struct buffer *buffer)
{
	struct buffer tmp;
	uint8_t extra;
	int ret;

	buffer_init_static(&tmp, buffer_data_current(buffer),
			   buffer_remain(buffer), false);

	ret = unpack_cbor_float(&tmp, &extra);
	if (ret)
		return ret;

	switch (extra) {
		case ADDL_FLOAT_BREAK:
			break;
		default:
			return -EILSEQ;
	}

	return sync_buffers(buffer, &tmp);
}

int cbor_unpack_map_start(struct buffer *buffer, uint64_t *npairs,
			  bool *end_required)
{
	struct buffer tmp;
	int ret;

	buffer_init_static(&tmp, buffer_data_current(buffer),
			   buffer_remain(buffer), false);

	ret = unpack_cbor_arraymap_start(&tmp, CMT_MAP, ADDL_MAP_INDEF,
					 npairs, end_required);
	if (ret)
		return ret;

	return sync_buffers(buffer, &tmp);
}

int cbor_unpack_map_end(struct buffer *buffer, bool end_required)
{
	if (!end_required)
		return 0;

	return cbor_unpack_break(buffer);
}

int cbor_unpack_array_start(struct buffer *buffer, uint64_t *nelem,
			    bool *end_required)
{
	struct buffer tmp;
	int ret;

	buffer_init_static(&tmp, buffer_data_current(buffer),
			   buffer_remain(buffer), false);

	ret = unpack_cbor_arraymap_start(&tmp, CMT_ARRAY, ADDL_ARRAY_INDEF,
					 nelem, end_required);
	if (ret)
		return ret;

	return sync_buffers(buffer, &tmp);
}

int cbor_unpack_array_end(struct buffer *buffer, bool end_required)
{
	if (!end_required)
		return 0;

	return cbor_unpack_break(buffer);
}

static struct val *unpack_cbor_null(struct buffer *buffer)
{
	int ret;

	ret = cbor_unpack_null(buffer);

	return ret ? ERR_PTR(ret) : VAL_ALLOC_NULL();
}

static struct val *unpack_cbor_uint(struct buffer *buffer)
{
	uint64_t tmp;
	int ret;

	ret = cbor_unpack_uint(buffer, &tmp);

	return ret ? ERR_PTR(ret) : val_alloc_int(tmp);
}

static struct val *unpack_cbor_str(struct buffer *buffer)
{
	struct str *str;
	int ret;

	ret = cbor_unpack_str(buffer, &str);

	return ret ? ERR_PTR(ret) : str_cast_to_val(str);
}

static struct val *unpack_cbor_bool(struct buffer *buffer)
{
	bool tmp;
	int ret;

	ret = cbor_unpack_bool(buffer, &tmp);

	return ret ? ERR_PTR(ret) : val_alloc_bool(tmp);
}

static struct val *unpack_cbor_blob(struct buffer *buffer)
{
	size_t size;
	void *data;
	int ret;

	ret = cbor_unpack_blob(buffer, &data, &size);

	return ret ? ERR_PTR(ret) : val_alloc_blob(data, size);
}

static struct val *unpack_cbor_array(struct buffer *buffer)
{
	bool end_required;
	struct val **arr;
	uint64_t nelem;
	int ret;

	ret = cbor_unpack_array_start(buffer, &nelem, &end_required);
	if (ret)
		return ERR_PTR(ret);

	arr = NULL;

	if (end_required) {
		/* indef */
		nelem = 0;

		for (;;) {
			struct val **tmparr;
			struct val *elem;

			ret = cbor_peek_break(buffer);
			if (!ret)
				break; /* end of array */
			if (ret != -EILSEQ)
				goto err; /* error */

			elem = unpack_cbor_val(buffer);
			if (IS_ERR(elem)) {
				ret = PTR_ERR(elem);
				goto err;
			}

			tmparr = mem_reallocarray(arr, nelem + 1,
						  sizeof(struct val *));
			if (!tmparr) {
				ret = -ENOMEM;
				goto err;
			}

			arr = tmparr;

			arr[nelem++] = elem;
		}
	} else {
		/* def */
		size_t i;

		if (nelem > SIZE_MAX)
			return ERR_PTR(-ENOMEM);

		arr = mem_recallocarray(NULL, 0, nelem, sizeof(struct val *));
		if (!arr)
			return ERR_PTR(-ENOMEM);

		for (i = 0; i < nelem; i++) {
			struct val *elem;

			elem = unpack_cbor_val(buffer);
			if (IS_ERR(elem)) {
				ret = PTR_ERR(elem);
				goto err;
			}

			arr[i] = elem;
		}
	}

	ret = cbor_unpack_array_end(buffer, end_required);
	if (ret)
		goto err;

	return val_alloc_array(arr, nelem);

err:
	if (arr) {
		size_t i;

		for (i = 0; i < nelem; i++)
			val_putref(arr[i]);

		free(arr);
	}

	return ERR_PTR(ret);
}

static int __unpack_cbor_pair(struct buffer *buffer, struct nvlist *nvl)
{
	struct nvpair pair;
	struct val *name;
	struct val *value;

	name = unpack_cbor_str(buffer);
	if (IS_ERR(name))
		return PTR_ERR(name);

	value = unpack_cbor_val(buffer);
	if (IS_ERR(value)) {
		val_putref(name);
		return PTR_ERR(value);
	}

	pair.name = val_cast_to_str(name);
	pair.value = value;

	return nvl_set_pair(nvl, &pair);
}

static struct val *unpack_cbor_nvl(struct buffer *buffer)
{
	struct nvlist *nvl;
	bool end_required;
	uint64_t npairs;
	int ret;

	ret = cbor_unpack_map_start(buffer, &npairs, &end_required);
	if (ret)
		return ERR_PTR(ret);

	nvl = nvl_alloc();
	if (IS_ERR(nvl))
		return ERR_CAST(nvl);

	if (end_required) {
		/* indef */
		for (;;) {
			ret = cbor_peek_break(buffer);
			if (!ret)
				break; /* end of map */
			if (ret != -EILSEQ)
				goto err; /* error */

			ret = __unpack_cbor_pair(buffer, nvl);
			if (ret)
				goto err;
		}
	} else {
		/* def */
		size_t i;

		if (npairs > SIZE_MAX) {
			ret = -ENOMEM;
			goto err;
		}

		for (i = 0; i < npairs; i++) {
			ret = __unpack_cbor_pair(buffer, nvl);
			if (ret)
				goto err;
		}
	}

	ret = cbor_unpack_map_end(buffer, end_required);
	if (ret)
		goto err;

	return nvl_cast_to_val(nvl);

err:
	nvl_putref(nvl);

	return ERR_PTR(ret);
}

static struct val *unpack_cbor_val(struct buffer *buffer)
{
	enum val_type type;
	int ret;

	ret = cbor_peek_type(buffer, &type);
	if (ret)
		return ERR_PTR(ret);

	switch (type) {
		case VT_NULL:
			return unpack_cbor_null(buffer);
		case VT_INT:
			return unpack_cbor_uint(buffer);
		case VT_STR:
			return unpack_cbor_str(buffer);
		case VT_SYM:
			return ERR_PTR(-ENOTSUP);
		case VT_BOOL:
			return unpack_cbor_bool(buffer);
		case VT_CONS:
			return ERR_PTR(-ENOTSUP);
		case VT_CHAR:
			return ERR_PTR(-ENOTSUP);
		case VT_BLOB:
			return unpack_cbor_blob(buffer);
		case VT_ARRAY:
			return unpack_cbor_array(buffer);
		case VT_NVL:
			return unpack_cbor_nvl(buffer);
	}

	return ERR_PTR(-ENOTSUP);
}

struct val *cbor_unpack_val(struct buffer *buffer)
{
	struct buffer tmp;
	struct val *val;
	int ret;

	buffer_init_static(&tmp, buffer_data_current(buffer),
			   buffer_remain(buffer), false);

	val = unpack_cbor_val(&tmp);
	if (IS_ERR(val))
		return val;

	ret = sync_buffers(buffer, &tmp);
	if (!ret)
		return val;

	val_putref(val);

	return ERR_PTR(ret);
}
