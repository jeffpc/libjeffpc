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
#include <jeffpc/json.h>
#include <jeffpc/nvl.h>

/*
 * pack
 */

int json_pack_uint(struct buffer *buffer, uint64_t v)
{
	char tmp[sizeof(v) * 2 * 2 + 1]; /* FIXME: hexdump would take 2x */

	snprintf(tmp, sizeof(tmp), "%"PRIu64, v);

	return buffer_append_cstr(buffer, tmp);
}

int json_pack_nint(struct buffer *buffer, uint64_t v)
{
	return -ENOTSUP; /* TODO */
}

int json_pack_int(struct buffer *buffer, int64_t v)
{
	if (v < 0)
		return json_pack_nint(buffer, -v);

	return json_pack_uint(buffer, v);
}

static int __escape_char(struct buffer *buffer, uint32_t c)
{
	char tmp[7];

	/* FIXME: char outside of basic multilingual plane */
	if (c > 0xffff)
		return -ENOTSUP;

	snprintf(tmp, sizeof(tmp), "\\u%04X", c);

	return buffer_append_cstr(buffer, tmp);
}

static int __escape_ctrl_char(struct buffer *buffer, uint8_t c)
{
	const char *mapped = NULL;

	switch (c) {
		case '\b':
			mapped = "\\b";
			break;
		case '\f':
			mapped = "\\f";
			break;
		case '\n':
			mapped = "\\n";
			break;
		case '\r':
			mapped = "\\r";
			break;
		case '\t':
			mapped = "\\t";
			break;
	}

	if (mapped)
		return buffer_append_cstr(buffer, mapped);

	return __escape_char(buffer, c);
}

int json_pack_cstr_len(struct buffer *buffer, const char *str, size_t len)
{
	size_t i;
	int ret;

	if ((ret = buffer_append_c(buffer, '"')))
		return ret;

	/* append the string... escaped */
	for (i = 0; i < len; i++) {
		uint8_t c = str[i];

		if (c <= 0x1f) {
			/* control character, must be escaped */
			ret = __escape_ctrl_char(buffer, c);
		} else if ((c == '"') || (c == '\\')) {
			/* quote or backslash */
			char tmp[3] = { '\\', c, '\0' };

			ret = buffer_append_cstr(buffer, tmp);
		} else {
			/* no escape necessary */
			ret = buffer_append_c(buffer, c);
		}

		if (ret)
			return ret;
	}

	if ((ret = buffer_append_c(buffer, '"')))
		return ret;

	return 0;
}

int json_pack_str(struct buffer *buffer, struct str *str)
{
	return json_pack_cstr_len(buffer, str_cstr(str), str_len(str));
}

int json_pack_bool(struct buffer *buffer, bool b)
{
	return buffer_append_cstr(buffer, b ? "true" : "false");
}

int json_pack_null(struct buffer *buffer)
{
	return buffer_append_cstr(buffer, "null");
}

int json_pack_array_start(struct buffer *buffer)
{
	return buffer_append_c(buffer, '[');
}

int json_pack_array_elem_sep(struct buffer *buffer)
{
	return buffer_append_c(buffer, ',');
}

int json_pack_array_end(struct buffer *buffer)
{
	return buffer_append_c(buffer, ']');
}

int json_pack_array_vals(struct buffer *buffer, struct val **vals, size_t nelem)
{
	size_t i;
	int ret;

	ret = json_pack_array_start(buffer);
	if (ret)
		return ret;

	for (i = 0; i < nelem; i++) {
		if (i) {
			ret = json_pack_array_elem_sep(buffer);
			if (ret)
				return ret;
		}

		ret = json_pack_val(buffer, vals[i]);
		if (ret)
			return ret;
	}

	return json_pack_array_end(buffer);
}

int json_pack_map_start(struct buffer *buffer)
{
	return buffer_append_c(buffer, '{');
}

int json_pack_map_name_sep(struct buffer *buffer)
{
	return buffer_append_c(buffer, ':');
}

int json_pack_map_pair_sep(struct buffer *buffer)
{
	return buffer_append_c(buffer, ',');
}

int json_pack_map_end(struct buffer *buffer)
{
	return buffer_append_c(buffer, '}');
}

int json_pack_map_val(struct buffer *buffer, struct val *val)
{
	struct rb_tree *tree = &val->_set_nvl.values;
	struct nvpair *cur;
	bool first;
	int ret;

	if (val->type != VT_NVL)
		return -EINVAL;

	ret = json_pack_map_start(buffer);
	if (ret)
		return ret;

	first = true;

	rb_for_each(tree, cur) {
		if (!first) {
			ret = json_pack_map_pair_sep(buffer);
			if (ret)
				return ret;
		} else {
			first = false;
		}

		ret = json_pack_str(buffer, cur->name);
		if (ret)
			return ret;

		ret = json_pack_map_name_sep(buffer);
		if (ret)
			return ret;

		ret = json_pack_val(buffer, cur->value);
		if (ret)
			return ret;
	}

	return json_pack_map_end(buffer);
}

int json_pack_val(struct buffer *buffer, struct val *val)
{
	switch (val->type) {
		case VT_NULL:
			return json_pack_null(buffer);
		case VT_INT:
			return json_pack_uint(buffer, val->i);
		case VT_STR:
			return json_pack_str(buffer, val_cast_to_str(val));
		case VT_SYM:
			return -ENOTSUP;
		case VT_BOOL:
			return json_pack_bool(buffer, val->b);
		case VT_CONS:
			return -ENOTSUP;
		case VT_CHAR:
			return -ENOTSUP;
		case VT_BLOB:
			return -ENOTSUP;
		case VT_ARRAY:
			/*
			 * We need to pass non-const struct vals so that we
			 * can use val_cast_to_*().
			 */
			return json_pack_array_vals(buffer, val->_set_array.vals,
						    val->array.nelem);
		case VT_NVL:
			return json_pack_map_val(buffer, val);
	}

	return -ENOTSUP;
}
