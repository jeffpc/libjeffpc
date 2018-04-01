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

#ifndef __JEFFPC_NVL_H
#define __JEFFPC_NVL_H

#include <stdbool.h>

#include <jeffpc/val.h>
#include <jeffpc/buffer.h>

struct nvlist {
	struct val val;
};

/* do not access these directly */
struct nvpair {
	struct bst_node node;

	struct str *name;
	struct val *value;
};

enum nvcvtcond {
	NVCVT_COND_ALWAYS = 0,
	NVCVT_COND_STR_EMPTY,
};

struct nvl_convert_info {
	const char *name;
	enum val_type tgt_type;
	enum nvcvtcond cond;
};

#define nvl_alloc()	((struct nvlist *) val_alloc_nvl())

/*
 * struct val reference counting & casting to struct nvl
 */

static inline struct nvlist *val_cast_to_nvl(struct val *val)
{
	ASSERT(!val || (val->type == VT_NVL));

	return container_of(val, struct nvlist, val);
}

#define val_getref_nvl(v)	val_cast_to_nvl(val_getref(v))

/*
 * struct nvl reference counting & casting to struct val
 */

#define nvl_getref(nvl)		((struct nvlist *) val_getref(nvl_cast_to_val(nvl)))

#define nvl_putref(nvl)		val_putref(nvl_cast_to_val(nvl))

#define nvl_getref_val(nvl)	val_getref(nvl_cast_to_val(nvl))

static inline struct val *nvl_cast_to_val(struct nvlist *nvl)
{
	return &nvl->val;
}

/*
 * Misc functions
 */

extern int nvl_merge(struct nvlist *dest, struct nvlist *src);

/*
 * If convert_all is true, then all errors except -ENOTSUP encountered
 * during the conversion are ignored.
 */
extern int nvl_convert(struct nvlist *nvl, const struct nvl_convert_info *table,
		       bool convert_all);

/* serialization & deserialization */
extern struct nvlist *nvl_unpack(const void *ptr, size_t len,
				 enum val_format format);

static inline ssize_t nvl_size(struct nvlist *nvl, enum val_format format)
{
	return val_size(nvl_cast_to_val(nvl), format);
}

static inline struct buffer *nvl_pack(struct nvlist *nvl,
				      enum val_format format)
{
	return val_pack(nvl_cast_to_val(nvl), format);
}

/* iteration */
extern const struct nvpair *nvl_iter_start(struct nvlist *nvl);
extern const struct nvpair *nvl_iter_next(struct nvlist *nvl,
					  const struct nvpair *prev);

#define nvl_for_each(pair, nvl)			\
	for (pair = nvl_iter_start(nvl);	\
	     pair != NULL;			\
	     pair = nvl_iter_next(nvl, pair))

/*
 * There are two types of lookup functions - those that return a pointer and
 * those than return an int.  The functions that return a pointer either
 * return a valid pointer with a new reference or an errno.  The functions
 * that return an int, return 0 and set the out argument to the value or
 * return an errno.
 *
 * -ENOENT = not found
 * -ERANGE = wrong type (e.g., looking up an int, but stored value is a string)
 */
extern int nvl_lookup_array(struct nvlist *nvl, const char *name,
			    struct val ***vals, size_t *nelem);
extern int nvl_lookup_blob(struct nvlist *nvl, const char *name,
			   const void **ptr, size_t *size);
extern int nvl_lookup_bool(struct nvlist *nvl, const char *name, bool *out);
extern int nvl_lookup_int(struct nvlist *nvl, const char *name, uint64_t *out);
extern int nvl_lookup_null(struct nvlist *nvl, const char *name);
extern const struct nvpair *nvl_lookup(struct nvlist *nvl, const char *name);
extern struct nvlist *nvl_lookup_nvl(struct nvlist *nvl, const char *name);
extern struct str *nvl_lookup_str(struct nvlist *nvl, const char *name);

/*
 * Add a new key-value pair or change the value of an existing key-value
 * pair.
 *
 * always consume val's reference
 *
 * -ENOMEM = out of memory
 */
extern int nvl_set(struct nvlist *nvl, const char *name, struct val *val);
extern int nvl_set_array(struct nvlist *nvl, const char *name,
			 struct val **vals, size_t nelem);
extern int nvl_set_array_copy(struct nvlist *nvl, const char *name,
			      struct val **vals, size_t nelem);
extern int nvl_set_blob(struct nvlist *nvl, const char *name, void *ptr,
			size_t size);
extern int nvl_set_blob_copy(struct nvlist *nvl, const char *name,
			     const void *ptr, size_t size);
extern int nvl_set_bool(struct nvlist *nvl, const char *name, bool val);
extern int nvl_set_cstr_dup(struct nvlist *nvl, const char *name,
			    const char *val);
extern int nvl_set_int(struct nvlist *nvl, const char *name, uint64_t val);
extern int nvl_set_null(struct nvlist *nvl, const char *name);
extern int nvl_set_nvl(struct nvlist *nvl, const char *name, struct nvlist *val);
extern int nvl_set_str(struct nvlist *nvl, const char *name, struct str *val);

/*
 * Remove a key-value pair from the list.
 *
 * -ENOENT = not found
 * -ERANGE = existing item's type doesn't match requested type
 */
extern int nvl_unset(struct nvlist *nvl, const char *name);
extern int nvl_unset_type(struct nvlist *nvl, const char *name, enum val_type type);

/*
 * Check existence of a specific key, or key-valuetype pair from the list.
 *
 * The generic nvl_exists returns true if the item exists and false
 * otherwise.
 *
 * The type-specific checks return:
 *
 * -ENOENT = does not exist
 * -ERANGE = exists, wrong type
 * 0       = exists, correct type
 */
extern bool nvl_exists(struct nvlist *nvl, const char *name);
extern int nvl_exists_type(struct nvlist *nvl, const char *name,
			   enum val_type type);

/*
 * nvpair related functions
 */

static inline const char *nvpair_name(const struct nvpair *pair)
{
	return str_cstr(pair->name);
}

static inline enum val_type nvpair_type(const struct nvpair *pair)
{
	return pair->value->type;
}

/*
 * As with the nvl_lookup_* functions, there return either a pointer or an
 * integer.  The meaning of the return values is the same as well.
 */
extern int nvpair_value_array(const struct nvpair *pair,
			      struct val ***vals, size_t *nelem);
extern int nvpair_value_blob(const struct nvpair *pair,
			     const void **ptr, size_t *size);
extern int nvpair_value_bool(const struct nvpair *pair, bool *out);
extern int nvpair_value_int(const struct nvpair *pair, uint64_t *out);
extern int nvpair_value_null(const struct nvpair *pair);
extern struct nvlist *nvpair_value_nvl(const struct nvpair *pair);
extern struct str *nvpair_value_str(const struct nvpair *pair);

#endif
