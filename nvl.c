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

#include <jeffpc/error.h>
#include <jeffpc/mem.h>
#include <jeffpc/nvl.h>

static void nvpair_free(struct nvpair *pair);

static struct mem_cache *nvlist_cache;
static struct mem_cache *nvpair_cache;

static void __attribute__((constructor)) init_nvl_subsys(void)
{
	nvlist_cache = mem_cache_create("nvlist-cache", sizeof(struct nvlist),
					0);
	ASSERT(!IS_ERR(nvlist_cache));
	nvpair_cache = mem_cache_create("nvpair-cache", sizeof(struct nvpair),
					0);
	ASSERT(!IS_ERR(nvpair_cache));
}

struct nvlist *nvl_alloc(void)
{
	struct nvlist *nvl;

	nvl = mem_cache_alloc(nvlist_cache);
	if (!nvl)
		return NULL;

	refcnt_init(&nvl->refcnt, 1);
	list_create(&nvl->values, sizeof(struct nvpair),
		    offsetof(struct nvpair, node));

	return nvl;
}

void nvl_free(struct nvlist *nvl)
{
	struct nvpair *cur, *safe;

	ASSERT(nvl);
	ASSERT3U(refcnt_read(&nvl->refcnt), ==, 0);

	list_for_each_safe(cur, safe, &nvl->values)
		nvpair_free(cur);

	mem_cache_free(nvlist_cache, nvl);
}

static void __nvval_release(struct nvval *val)
{
	size_t i;

	switch (val->type) {
		case NVT_ARRAY:
			for (i = 0; i < val->array.nelem; i++)
				__nvval_release(&val->array.vals[i]);
			free(val->array.vals);
			break;
		case NVT_BLOB:
			free(val->blob.ptr);
			break;
		case NVT_BOOL:
		case NVT_INT:
		case NVT_NULL:
			break;
		case NVT_NVL:
			nvl_putref(val->nvl);
			break;
		case NVT_STR:
			str_putref(val->str);
			break;
	}
}

void nvval_release_array(struct nvval *vals, size_t nelem)
{
	size_t i;

	if (!vals && !nelem)
		return;

	ASSERT(vals);
	ASSERT(nelem);

	for (i = 0; i < nelem; i++)
		__nvval_release(&vals[i]);
}

static struct nvval *val_dup_array(const struct nvval *user_vals, size_t nelem)
{
	struct nvval *vals;
	size_t i;

#ifdef HAVE_REALLOCARRAY
	vals = reallocarray(NULL, nelem, sizeof(struct nvval));
#else
	vals = malloc(nelem * sizeof(struct nvval));
#endif
	if (!vals)
		return NULL;

	for (i = 0; i < nelem; i++) {
		const struct nvval *uval = &user_vals[i];

		vals[i].type = uval->type;

		switch (uval->type) {
			case NVT_ARRAY: {
				struct nvval *subvals;

				subvals = val_dup_array(uval->array.vals,
							uval->array.nelem);
				if (!subvals)
					goto err;

				vals[i].array.vals = subvals;
				vals[i].array.nelem = uval->array.nelem;
				break;
			}
			case NVT_BLOB: {
				void *tmp;

				tmp = malloc(uval->blob.size);
				if (!tmp)
					goto err;

				memcpy(tmp, uval->blob.ptr, uval->blob.size);

				vals[i].blob.ptr = tmp;
				vals[i].blob.size = uval->blob.size;
				break;
			}
			case NVT_BOOL:
				vals[i].b = uval->b;
				break;
			case NVT_INT:
				vals[i].i = uval->i;
				break;
			case NVT_NULL:
				break;
			case NVT_NVL:
				vals[i].nvl = nvl_getref(uval->nvl);
				break;
			case NVT_STR:
				vals[i].str = str_getref(uval->str);
				break;
		}
	}

	return vals;

err:
	/* only free the copied elements */
	nelem = i;

	nvval_release_array(vals, nelem);
	free(vals);

	return NULL;
}

static struct nvpair *nvpair_alloc(const char *name)
{
	struct nvpair *pair;

	pair = mem_cache_alloc(nvpair_cache);
	if (!pair)
		return NULL;

	pair->name = strdup(name);
	if (!pair->name) {
		mem_cache_free(nvpair_cache, pair);
		return NULL;
	}

	pair->value.type = NVT_INT;
	pair->value.i = 0;

	return pair;
}

static void nvpair_free(struct nvpair *pair)
{
	__nvval_release(&pair->value);

	free((char *) pair->name);

	mem_cache_free(nvpair_cache, pair);
}

int nvl_merge(struct nvlist *dest, struct nvlist *src)
{
	const struct nvpair *spair;

	nvl_for_each(spair, src) {
		const struct nvval *val = &spair->value;
		int ret;

		switch (spair->value.type) {
			case NVT_ARRAY:
				ret = nvl_set_array_copy(dest, spair->name,
							 val->array.vals,
							 val->array.nelem);
				break;
			case NVT_BLOB:
				ret = nvl_set_blob_copy(dest, spair->name,
							val->blob.ptr,
							val->blob.size);
				break;
			case NVT_BOOL:
				ret = nvl_set_bool(dest, spair->name, val->b);
				break;
			case NVT_INT:
				ret = nvl_set_int(dest, spair->name, val->i);
				break;
			case NVT_NULL:
				ret = nvl_set_null(dest, spair->name);
				break;
			case NVT_NVL:
				ret = nvl_set_nvl(dest, spair->name,
						  nvl_getref(val->nvl));
				break;
			case NVT_STR:
				ret = nvl_set_str(dest, spair->name,
						  str_getref(val->str));
				break;
		}

		if (ret)
			return ret;
	}

	return 0;
}

static struct nvpair *find(struct nvlist *nvl, const char *name)
{
	struct nvpair *cur;

	list_for_each(cur, &nvl->values)
		if (!strcmp(cur->name, name))
			return cur;

	return NULL;
}

/*
 * nvlist iteration
 */

const struct nvpair *nvl_iter_next(struct nvlist *nvl,
				   const struct nvpair *prev)
{
	return list_next(&nvl->values, (struct nvpair *) prev);
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
		     const struct nvval **vals, size_t *nelem)
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

static struct nvpair *__nvl_set_prep(struct nvlist *nvl, const char *name)
{
	struct nvpair *pair;

	pair = find(nvl, name);
	if (!pair) {
		/* not found - allocate a new pair */
		pair = nvpair_alloc(name);
		if (!pair)
			return NULL;

		list_insert_tail(&nvl->values, pair);
	} else {
		/* found & should reuse it */
		__nvval_release(&pair->value);

		pair->value.type = NVT_INT;
		pair->value.i = 0;
	}

	return pair;
}

#define SET(fxn, ctype, nvtype, nvmember, putref)			\
int fxn(struct nvlist *nvl, const char *name, ctype val)		\
{									\
	struct nvpair *pair;						\
									\
	pair = __nvl_set_prep(nvl, name);				\
	if (!pair) {							\
		putref(val);						\
		return -ENOMEM;						\
	}								\
									\
	pair->value.type = nvtype;					\
	pair->value.nvmember = val;					\
									\
	return 0;							\
}

int nvl_set_array(struct nvlist *nvl, const char *name,
		  struct nvval *vals, size_t nelem)
{
	struct nvpair *pair;

	pair = __nvl_set_prep(nvl, name);
	if (!pair)
		return -ENOMEM;

	pair->value.type = NVT_ARRAY;
	pair->value.array.vals = vals;
	pair->value.array.nelem = nelem;

	return 0;
}

int nvl_set_array_copy(struct nvlist *nvl, const char *name,
		       const struct nvval *vals, size_t nelem)
{
	struct nvval *tmp;
	int ret;

	tmp = val_dup_array(vals, nelem);
	if (!tmp)
		return -ENOMEM;

	ret = nvl_set_array(nvl, name, tmp, nelem);
	if (ret) {
		nvval_release_array(tmp, nelem);
		free(tmp);
	}

	return ret;
}

int nvl_set_blob(struct nvlist *nvl, const char *name, void *ptr, size_t size)
{
	struct nvpair *pair;

	pair = __nvl_set_prep(nvl, name);
	if (!pair)
		return -ENOMEM;

	pair->value.type = NVT_BLOB;
	pair->value.blob.ptr = ptr;
	pair->value.blob.size = size;

	return 0;
}

int nvl_set_blob_copy(struct nvlist *nvl, const char *name, const void *ptr,
		      size_t size)
{
	void *tmp;
	int ret;

	tmp = malloc(size);
	if (!tmp)
		return -ENOMEM;

	memcpy(tmp, ptr, size);

	ret = nvl_set_blob(nvl, name, tmp, size);
	if (ret)
		free(tmp);

	return ret;

}

SET(nvl_set_bool, bool, NVT_BOOL, b, (void));

int nvl_set_cstr_dup(struct nvlist *nvl, const char *name, const char *val)
{
	struct str *str;

	str = str_dup(val);
	if (!str)
		return -ENOMEM;

	return nvl_set_str(nvl, name, str);
}

SET(nvl_set_int, uint64_t, NVT_INT, i, (void));

int nvl_set_null(struct nvlist *nvl, const char *name)
{
	struct nvpair *pair;

	pair = __nvl_set_prep(nvl, name);
	if (!pair)
		return -ENOMEM;

	pair->value.type = NVT_NULL;

	return 0;
}

SET(nvl_set_nvl, struct nvlist *, NVT_NVL, nvl, nvl_putref);
SET(nvl_set_str, struct str *, NVT_STR, str, str_putref);

/*
 * nvlist unset
 */

static int unset(struct nvlist *nvl, const char *name, enum nvtype type,
		 bool matchtype)
{
	struct nvpair *pair;

	pair = find(nvl, name);
	if (!pair)
		return -ENOENT;

	if (matchtype && (pair->value.type != type))
		return -ERANGE;

	list_remove(&nvl->values, pair);

	nvpair_free(pair);

	return 0;
}

int nvl_unset(struct nvlist *nvl, const char *name)
{
	return unset(nvl, name, NVT_NULL, false);
}

int nvl_unset_type(struct nvlist *nvl, const char *name, enum nvtype type)
{
	return unset(nvl, name, type, true);
}

/*
 * nvpair value
 */

#define VALUE_INT(fxn, ctype, nvtype, nvmember)				\
int fxn(const struct nvpair *pair, ctype *out)				\
{									\
	if (!pair)							\
		return -ENOENT;						\
									\
	if (pair->value.type != nvtype)					\
		return -ERANGE;						\
									\
	*out = pair->value.nvmember;					\
									\
	return 0;							\
}

#define VALUE_PTR(fxn, ctype, nvtype, nvmember, getref)			\
ctype fxn(const struct nvpair *pair)					\
{									\
	if (!pair)							\
		return ERR_PTR(-ENOENT);				\
									\
	if (pair->value.type != nvtype)					\
		return ERR_PTR(-ERANGE);				\
									\
	return getref(pair->value.nvmember);				\
}

int nvpair_value_array(const struct nvpair *pair, const struct nvval **vals,
		       size_t *nelem)
{
	if (!pair)
		return -ENOENT;

	if (pair->value.type != NVT_ARRAY)
		return -ERANGE;

	*vals = pair->value.array.vals;
	*nelem = pair->value.array.nelem;

	return 0;
}

int nvpair_value_blob(const struct nvpair *pair, const void **ptr, size_t *size)
{
	if (!pair)
		return -ENOENT;

	if (pair->value.type != NVT_BLOB)
		return -ERANGE;

	*ptr = pair->value.blob.ptr;
	*size = pair->value.blob.size;

	return 0;
}

VALUE_INT(nvpair_value_bool, bool, NVT_BOOL, b);
VALUE_INT(nvpair_value_int, uint64_t, NVT_INT, i);

int nvpair_value_null(const struct nvpair *pair)
{
	if (!pair)
		return -ENOENT;

	if (pair->value.type != NVT_NULL)
		return -ERANGE;

	return 0;
}

VALUE_PTR(nvpair_value_nvl, struct nvlist *, NVT_STR, nvl, nvl_getref);
VALUE_PTR(nvpair_value_str, struct str *, NVT_STR, str, str_getref);
