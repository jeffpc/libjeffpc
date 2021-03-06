/*
 * Copyright (c) 2014-2017 Josef 'Jeff' Sipek <jeffpc@josefsipek.net>
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

/*
 * Check if a condition is true.  Returns:
 *
 * 0 if true
 * -EBUSY if false
 * -EINVAL if args are wrong
 */
static int check_condition_str(struct str *str, enum nvcvtcond cond)
{
	switch (cond) {
		case NVCVT_COND_ALWAYS:
			return 0;
		case NVCVT_COND_STR_EMPTY:
			return str_len(str) ? -EBUSY : 0;
	}

	return -EINVAL;
}

static int cvt_string(struct nvlist *nvl, const struct nvpair *pair,
		      enum val_type tgt, enum nvcvtcond cond)
{
	const char *name = nvpair_name(pair);
	struct str *str;
	uint64_t intval;
	int ret = -ENOTSUP;

	ASSERT3U(nvpair_type(pair), ==, VT_STR);

	str = nvpair_value_str(pair);
	if (IS_ERR(str)) {
		VERIFY3S(PTR_ERR(str), !=, -ENOENT);
		VERIFY3S(PTR_ERR(str), !=, -ERANGE);
		return PTR_ERR(str);
	}

	/* check if the specified condition is true */
	ret = check_condition_str(str, cond);
	if (ret != 0) {
		str_putref(str);
		return (ret == -EBUSY) ? 0 : ret;
	}

	switch (tgt) {
		case VT_ARRAY:
		case VT_BLOB:
		case VT_BOOL:
		case VT_SYM:
		case VT_CONS:
		case VT_CHAR:
			ret = -ENOTSUP;
			break;
		case VT_INT:
			ret = str2u64(str_cstr(str), &intval);
			if (!ret)
				ret = nvl_set_int(nvl, name, intval);
			break;
		case VT_NULL:
			ret = nvl_set_null(nvl, name);
			break;
		case VT_NVL:
			ret = -ENOTSUP;
			break;
		case VT_STR:
			/* nothing to do */
			ret = 0;
			break;
	}

	str_putref(str);

	return ret;
}

#define CHECK_ERROR(e, cvtall) \
	((!(cvtall) && (e)) || \
	 ((cvtall) && ((e) == -ENOTSUP)))

int nvl_convert(struct nvlist *nvl, const struct nvl_convert_info *table,
		bool convert_all)
{
	/*
	 * If we weren't given a table, we have nothing to do.
	 */
	if (!table)
		return 0;

	for (; table->name; table++) {
		const struct nvpair *pair;
		int ret = -ENOTSUP;

		pair = nvl_lookup(nvl, table->name);
		if (IS_ERR(pair)) {
			ret = PTR_ERR(pair);

			if ((ret == -ENOENT) || !CHECK_ERROR(ret, convert_all))
				continue;

			return ret;
		}

		switch (nvpair_type(pair)) {
			case VT_ARRAY:
			case VT_BLOB:
			case VT_BOOL:
			case VT_INT:
			case VT_NULL:
			case VT_NVL:
			case VT_SYM:
			case VT_CONS:
			case VT_CHAR:
				return -ENOTSUP;
			case VT_STR:
				ret = cvt_string(nvl, pair, table->tgt_type,
						 table->cond);
				break;
		}

		if (CHECK_ERROR(ret, convert_all))
			return ret;
	}

	return 0;
}
