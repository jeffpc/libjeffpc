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

#include <jeffpc/error.h>
#include <jeffpc/mem.h>
#include <jeffpc/nvl.h>

#include "val_impl.h"

int nvl_merge(struct nvlist *dest, struct nvlist *src)
{
	const struct nvpair *spair;

	nvl_for_each(spair, src) {
		int ret;

		ret = nvl_set(dest, str_cstr(spair->name), spair->value);
		if (ret)
			return ret;
	}

	return 0;
}

static struct nvpair *find(struct nvlist *nvl, const char *name)
{
	struct str name_str = STR_STATIC_INITIALIZER(name);
	struct nvpair key = {
		.name = &name_str,
	};

	return bst_find(&nvl->val._set_nvl.values, &key, NULL);
}

/*
 * nvlist iteration
 */

const struct nvpair *nvl_iter_start(struct nvlist *nvl)
{
	return bst_first(&nvl->val._set_nvl.values);
}

const struct nvpair *nvl_iter_next(struct nvlist *nvl,
				   const struct nvpair *prev)
{
	return bst_next(&nvl->val._set_nvl.values, (void *) prev);
}

/*
 * nvlist lookup
 */

#define LOOKUP_INT(fxn, ctype, pairfxn)					\
int fxn(struct nvlist *nvl, const char *name, ctype *out)		\
{									\
	return pairfxn(find(nvl, name), out);				\
}

#define LOOKUP_PTR(fxn, ctype, pairfxn)					\
ctype fxn(struct nvlist *nvl, const char *name)				\
{									\
	return pairfxn(find(nvl, name));				\
}

const struct nvpair *nvl_lookup(struct nvlist *nvl, const char *name)
{
	struct nvpair *pair;

	pair = find(nvl, name);

	return pair ? pair : ERR_PTR(-ENOENT);
}

int nvl_lookup_array(struct nvlist *nvl, const char *name,
		     struct val ***vals, size_t *nelem)
{
	return nvpair_value_array(find(nvl, name), vals, nelem);
}

int nvl_lookup_blob(struct nvlist *nvl, const char *name,
		    const void **ptr, size_t *size)
{
	return nvpair_value_blob(find(nvl, name), ptr, size);
}

LOOKUP_INT(nvl_lookup_bool, bool, nvpair_value_bool);
LOOKUP_INT(nvl_lookup_int, uint64_t, nvpair_value_int);

int nvl_lookup_null(struct nvlist *nvl, const char *name)
{
	return nvpair_value_null(find(nvl, name));
}

LOOKUP_PTR(nvl_lookup_nvl, struct nvlist *, nvpair_value_nvl);
LOOKUP_PTR(nvl_lookup_str, struct str *, nvpair_value_str);

/*
 * nvlist set
 */

int nvl_set(struct nvlist *nvl, const char *name, struct val *val)
{
	struct nvpair *pair;

	pair = find(nvl, name);
	if (!pair) {
		/* not found - allocate a new pair */
		pair = __nvpair_alloc(name);
		if (!pair) {
			val_putref(val);
			return -ENOMEM;
		}

		bst_add(&nvl->val._set_nvl.values, pair);
	}

	val_putref(pair->value);
	pair->value = val;

	return 0;
}

#define SET(nvl, name, valalloc)					\
	do {								\
		struct val *val;					\
									\
		val = valalloc;						\
		if (IS_ERR(val))					\
			return PTR_ERR(val);				\
									\
		return nvl_set(nvl, name, val);				\
	} while (0)

#define SET_ARG0(fxn, valalloc)						\
int fxn(struct nvlist *nvl, const char *name)				\
{									\
	SET(nvl, name, valalloc());					\
}

#define SET_ARG1(fxn, ctype, valalloc)					\
int fxn(struct nvlist *nvl, const char *name, ctype v)			\
{									\
	SET(nvl, name, valalloc(v));					\
}

#define SET_ARG2(fxn, ctype1, ctype2, valalloc)				\
int fxn(struct nvlist *nvl, const char *name, ctype1 a, ctype2 b)	\
{									\
	SET(nvl, name, valalloc(a, b));					\
}

SET_ARG2(nvl_set_array, struct val **, size_t, val_alloc_array);
SET_ARG2(nvl_set_array_copy, struct val **, size_t, val_alloc_array_dup);
SET_ARG2(nvl_set_blob, void *, size_t, val_alloc_blob);
SET_ARG2(nvl_set_blob_copy, const void *, size_t, val_alloc_blob_dup);
SET_ARG1(nvl_set_bool, bool, val_alloc_bool);
SET_ARG1(nvl_set_cstr_dup, const char *, val_dup_str);
SET_ARG1(nvl_set_int, uint64_t, val_alloc_int);
SET_ARG0(nvl_set_null, val_alloc_null);
SET_ARG1(nvl_set_nvl, struct nvlist *, nvl_cast_to_val);
SET_ARG1(nvl_set_str, struct str *, str_cast_to_val);

/*
 * nvlist unset
 */

static int unset(struct nvlist *nvl, const char *name, enum val_type type,
		 bool matchtype)
{
	struct nvpair *pair;

	pair = find(nvl, name);
	if (!pair)
		return -ENOENT;

	if (matchtype && (pair->value->type != type))
		return -ERANGE;

	bst_remove(&nvl->val._set_nvl.values, pair);

	__nvpair_free(pair);

	return 0;
}

int nvl_unset(struct nvlist *nvl, const char *name)
{
	return unset(nvl, name, VT_NULL, false);
}

int nvl_unset_type(struct nvlist *nvl, const char *name, enum val_type type)
{
	return unset(nvl, name, type, true);
}

/*
 * nvlist exists
 */
bool nvl_exists(struct nvlist *nvl, const char *name)
{
	return find(nvl, name) != NULL;
}

int nvl_exists_type(struct nvlist *nvl, const char *name, enum val_type type)
{
	struct nvpair *pair;

	pair = find(nvl, name);
	if (!pair)
		return -ENOENT;

	if (pair->value->type != type)
		return -ERANGE;

	return 0;
}

/*
 * nvpair value
 */

#define VALUE_INT(fxn, ctype, nvtype, valmember)			\
int fxn(const struct nvpair *pair, ctype *out)				\
{									\
	if (!pair)							\
		return -ENOENT;						\
									\
	if (pair->value->type != nvtype)				\
		return -ERANGE;						\
									\
	*out = pair->value->valmember;					\
									\
	return 0;							\
}

#define VALUE_PTR(fxn, ctype, valtype, getref)				\
ctype fxn(const struct nvpair *pair)					\
{									\
	if (!pair)							\
		return ERR_PTR(-ENOENT);				\
									\
	if (pair->value->type != valtype)				\
		return ERR_PTR(-ERANGE);				\
									\
	return getref(pair->value);					\
}

int nvpair_value_array(const struct nvpair *pair, struct val ***vals,
		       size_t *nelem)
{
	if (!pair)
		return -ENOENT;

	if (pair->value->type != VT_ARRAY)
		return -ERANGE;

	*vals = pair->value->_set_array.vals;
	*nelem = pair->value->array.nelem;

	return 0;
}

int nvpair_value_blob(const struct nvpair *pair, const void **ptr, size_t *size)
{
	if (!pair)
		return -ENOENT;

	if (pair->value->type != VT_BLOB)
		return -ERANGE;

	*ptr = pair->value->blob.ptr;
	*size = pair->value->blob.size;

	return 0;
}

VALUE_INT(nvpair_value_bool, bool, VT_BOOL, b);
VALUE_INT(nvpair_value_int, uint64_t, VT_INT, i);

int nvpair_value_null(const struct nvpair *pair)
{
	if (!pair)
		return -ENOENT;

	if (pair->value->type != VT_NULL)
		return -ERANGE;

	return 0;
}

VALUE_PTR(nvpair_value_nvl, struct nvlist *, VT_NVL, val_getref_nvl);
VALUE_PTR(nvpair_value_str, struct str *, VT_STR, val_getref_str);
