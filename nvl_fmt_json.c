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

static int json_nvl_prologue(struct buffer *buffer, struct nvlist *nvl)
{
	return buffer_append_c(buffer, '{');
}

static int json_nvl_name_sep(struct buffer *buffer, const struct nvpair *pair)
{
	return buffer_append_c(buffer, ':');
}

static int json_nvl_val_sep(struct buffer *buffer, const struct nvpair *pair)
{
	return buffer_append_c(buffer, ',');
}

static int json_nvl_epilogue(struct buffer *buffer, struct nvlist *nvl)
{
	return buffer_append_c(buffer, '}');
}

static int json_array_prologue(struct buffer *buffer, const struct nvval *vals,
			       size_t nelem)
{
	return buffer_append_c(buffer, '[');
}

static int json_array_val_sep(struct buffer *buffer, const struct nvval *val)
{
	return buffer_append_c(buffer, ',');
}

static int json_array_epilogue(struct buffer *buffer, const struct nvval *vals,
			       size_t nelem)
{
	return buffer_append_c(buffer, ']');
}

static int json_val_bool(struct buffer *buffer, bool b)
{
	return buffer_append_str(buffer, b ? "true" : "false");
}

static int json_val_int(struct buffer *buffer, uint64_t i)
{
	char tmp[sizeof(i) * 2 * 2 + 1]; /* FIXME: hexdump would take 2x */

	snprintf(tmp, sizeof(tmp), "%"PRIu64, i);

	return buffer_append_str(buffer, tmp);
}

static int json_val_null(struct buffer *buffer)
{
	return buffer_append_str(buffer, "null");
}

static int __escape_char(struct buffer *buffer, uint64_t c)
{
	char tmp[7];

	/* FIXME: char outside of basic multilingual plane */
	if (c > 0xffff)
		return -ENOTSUP;

	snprintf(tmp, sizeof(tmp), "\\u%04"PRIX64, c);

	return buffer_append_str(buffer, tmp);
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
		return buffer_append_str(buffer, mapped);

	return __escape_char(buffer, c);
}

static int json_val_str(struct buffer *buffer, const char *str)
{
	int ret;

	if ((ret = buffer_append_c(buffer, '"')))
		return ret;

	/* append the string... escaped */
	for (; *str; str++) {
		uint8_t c = *str;

		if (c <= 0x1f) {
			/* control character, must be escaped */
			ret = __escape_ctrl_char(buffer, c);
		} else if ((c == '"') || (c == '\\')) {
			/* quote or backslash */
			char tmp[3] = { '\\', c, '\0' };

			ret = buffer_append_str(buffer, tmp);
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

const struct nvops nvops_json = {
	.pack = {
		.nvl_prologue = json_nvl_prologue,	/* { */
		.nvl_name_sep = json_nvl_name_sep,	/* : */
		.nvl_val_sep = json_nvl_val_sep,	/* , */
		.nvl_epilogue = json_nvl_epilogue,	/* } */

		.array_prologue = json_array_prologue,	/* [ */
		.array_val_sep = json_array_val_sep,	/* , */
		.array_epilogue = json_array_epilogue,	/* ] */

		.val_bool = json_val_bool,
		.val_int = json_val_int,
		.val_null = json_val_null,
		.val_str = json_val_str,
	}
};
