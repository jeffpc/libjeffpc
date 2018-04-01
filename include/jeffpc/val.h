/*
 * Copyright (c) 2014-2018 Josef 'Jeff' Sipek <jeffpc@josefsipek.net>
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

#ifndef __JEFFPC_VAL_H
#define __JEFFPC_VAL_H

#include <stdint.h>
#include <stdbool.h>

#include <jeffpc/int.h>
#include <jeffpc/refcnt.h>
#include <jeffpc/error.h>
#include <jeffpc/types.h>
#include <jeffpc/list.h>
#include <jeffpc/bst.h>

/*
 * A typed value structure.
 *
 * A struct val can hold a number of different values along with type
 * information.  There are several different types that are supported.
 *
 * Note: The sexpr code considers NULL the same as an empty VT_CONS.  This
 * is different from VT_NULL.
 *
 * Note: Not all consumers of struct val use all types or can reliably
 * serialize and deserialize all types.
 */

enum val_type {
	VT_NULL = 0,	/* not a value */
	VT_INT,		/* 64-bit uint */
	VT_STR,		/* a struct str string */
	VT_SYM,		/* symbol */
	VT_BOOL,	/* boolean */
	VT_CONS,	/* cons cell */
	VT_CHAR,	/* a single (unicode) character */
	VT_BLOB,	/* a byte string */
	VT_ARRAY,	/* an array of values */
	VT_NVL,		/* an nvlist */
};

/* serialization formats */
enum val_format {
	VF_CBOR,	/* RFC 7049 */
	VF_JSON,	/* RFC 7159 */
};

#define STR_INLINE_LEN	15

struct val {
	enum val_type type;
	refcnt_t refcnt;
	bool static_struct:1;	/* struct statically allocated */
	bool static_alloc:1;	/* pointer is static */
	bool inline_alloc:1;	/* data is inline */
	union {
		const uint64_t i;
		const bool b;
		const char *str_ptr;
		const char str_inline[STR_INLINE_LEN + 1];
		const struct {
			struct val *head;
			struct val *tail;
		} cons;
		const struct {
			const void *ptr;
			size_t size;
		} blob;
		const struct {
			struct val **vals;
			size_t nelem;
		} array;
		const struct {
			/*
			 * TODO: Using an unbalanced binary search tree is
			 * not great, but it will do for now.  Once we have
			 * a balanced binary search tree implementation, we
			 * should switch to it.
			 */
			struct bst_tree values;
		} nvl;

		/*
		 * We want to keep the normal members const to catch
		 * attempts to modify them, but at the same time we need to
		 * initialize them after allocation.  Instead of venturing
		 * into undefined behavior territory full of ugly casts, we
		 * just duplicate the above members without the const and
		 * with much uglier name.
		 *
		 * Do not use the following members unless you are the val
		 * allocation function!
		 */
		uint64_t _set_i;
		bool _set_b;
		const char *_set_str_ptr;
		char _set_str_inline[STR_INLINE_LEN + 1];
		struct {
			struct val *head;
			struct val *tail;
		} _set_cons;
		struct {
			void *ptr;
			size_t size;
		} _set_blob;
		struct {
			struct val **vals;
			size_t nelem;
		} _set_array;
		struct {
			struct bst_tree values;
		} _set_nvl;
	};
};

/* ref-counted string */
struct str {
	struct val val;
};

/* ref-counted symbol name */
struct sym {
	struct val val;
};

/*
 * Allocation functions
 */

extern struct val *val_alloc_blob(void *ptr, size_t size);
extern struct val *val_alloc_blob_dup(const void *ptr, size_t size);
extern struct val *val_alloc_blob_static(const void *ptr, size_t size);
extern struct val *val_alloc_bool(bool v);
extern struct val *val_alloc_char(uint64_t v);
extern struct val *val_alloc_int(uint64_t v);
extern struct val *val_alloc_null(void);
extern struct val *val_alloc_nvl(void);

/* val_alloc_cons always consume the passed in references */
extern struct val *val_alloc_cons(struct val *head, struct val *tail);

/* assorted preallocated values */
extern struct val *val_empty_cons(void);

/*
 * The passed in struct val references are always consumed.
 *
 * val_alloc_array takes consumes the heap allocated vals array
 * val_alloc_array_dup duplicates the vals array
 * val_alloc_array_static uses the vals array as-is
 */
extern struct val *val_alloc_array(struct val **vals, size_t nelem);
extern struct val *val_alloc_array_dup(struct val **vals, size_t nelem);
extern struct val *val_alloc_array_static(struct val **vals, size_t nelem);

/*
 * Passed in string must be freed by the str code.  (I.e., we're adopting
 * it.)
 */
#define str_alloc(s)		((struct str *) _strsym_alloc((s), VT_STR))
#define sym_alloc(s)		((struct sym *) _strsym_alloc((s), VT_SYM))
#define val_alloc_str(s)	_strsym_alloc((s), VT_STR)
#define val_alloc_sym(s)	_strsym_alloc((s), VT_SYM)
extern struct val *_strsym_alloc(char *s, enum val_type type);
/*
 * Passed in str cannot be freed, and it doesn't have to be dup'd.  (E.g.,
 * it could be a string in .rodata.)
 */
#define str_alloc_static(s)	((struct str *) _strsym_alloc_static((s), VT_STR))
#define sym_alloc_static(s)	((struct sym *) _strsym_alloc_static((s), VT_SYM))
#define val_alloc_str_static(s)	_strsym_alloc_static((s), VT_STR)
#define val_alloc_sym_static(s)	_strsym_alloc_static((s), VT_SYM)
extern struct val *_strsym_alloc_static(const char *s, enum val_type type);
/*
 * Passed in string cannot be freed, and it must be dup'd.  (E.g., it could
 * be a string on the stack.)
 */
#define str_dup(s)		((struct str *) _strsym_dup((s), VT_STR))
#define sym_dup(s)		((struct sym *) _strsym_dup((s), VT_SYM))
#define val_dup_str(s)		_strsym_dup((s), VT_STR)
#define val_dup_sym(s)		_strsym_dup((s), VT_SYM)
extern struct val *_strsym_dup(const char *s, enum val_type type);
/*
 * Like above, but instead of a nul-terminated string use the passed in
 * length.
 */
#define str_dup_len(s, l)	((struct str *) _strsym_dup_len((s), (l), VT_STR))
#define sym_dup_len(s, l)	((struct sym *) _strsym_dup_len((s), (l), VT_SYM))
#define val_dup_str_len(s, l)	_strsym_dup_len((s), (l), VT_STR)
#define val_dup_sym_len(s, l)	_strsym_dup_len((s), (l), VT_SYM)
extern struct val *_strsym_dup_len(const char *s, size_t len,
				   enum val_type type);

/* assorted preallocated strings */
extern struct str *str_empty_string(void);

/*
 * Free functions
 */

extern void val_free(struct val *v);

/*
 * Dumping functions
 */

extern void val_dump_file(FILE *out, struct val *v, int indent);

static inline void val_dump(struct val *v, int indent)
{
	val_dump_file(stderr, v, indent);
}

/*
 * Misc functions
 */

#define str_len(s)	_strsym_len(&(s)->val)
#define sym_len(s)	_strsym_len(&(s)->val)
extern size_t _strsym_len(const struct val *val);

#define str_cmp(a, b)	_strsym_cmp(&(a)->val, &(b)->val)
#define sym_cmp(a, b)	_strsym_cmp(&(a)->val, &(b)->val)
extern int _strsym_cmp(const struct val *a, const struct val *b);

extern struct str *str_cat(size_t n, ...);
extern struct str *str_printf(const char *fmt, ...)
	__attribute__((format (printf, 1, 2)));
extern struct str *str_vprintf(const char *fmt, va_list args);

/*
 * struct val reference counting & casting to struct str/sym
 */

static inline bool val_isstatic(struct val *x)
{
	return x->static_struct;
}

REFCNT_INLINE_FXNS(struct val, val, refcnt, val_free, val_isstatic);

#define val_cast_to_str(v)	((struct str *) _val_cast_to_strsym(v, VT_STR))
#define val_cast_to_sym(v)	((struct sym *) _val_cast_to_strsym(v, VT_SYM))
static inline struct val *_val_cast_to_strsym(struct val *val,
					      enum val_type type)
{
	ASSERT(!val || (val->type == type));

	return val_getref(val);
}

#define val_getref_str(v)	val_cast_to_str(val_getref(v))
#define val_getref_sym(v)	val_cast_to_sym(val_getref(v))

/*
 * struct str/sym reference counting & casting to struct val
 */

#define str_getref(s)		((struct str *) val_getref(str_cast_to_val(s)))
#define sym_getref(s)		((struct sym *) val_getref(sym_cast_to_val(s)))

#define str_putref(s)		val_putref(str_cast_to_val(s))
#define sym_putref(s)		val_putref(sym_cast_to_val(s))

#define str_getref_val(s)	val_getref(str_cast_to_val(s))
#define sym_getref_val(s)	val_getref(sym_cast_to_val(s))

static inline struct val *str_cast_to_val(struct str *str)
{
	return &str->val;
}

static inline struct val *sym_cast_to_val(struct sym *sym)
{
	return &sym->val;
}

/*
 * struct val/str/sym casting to cstr
 */

static inline const char *val_cstr(const struct val *val)
{
	if (!val)
		return NULL;

	VERIFY((val->type == VT_STR) || (val->type == VT_SYM));

	return val->inline_alloc ? val->str_inline : val->str_ptr;
}

static inline const char *str_cstr(const struct str *str)
{
	return val_cstr(&str->val);
}

static inline const char *sym_cstr(const struct sym *sym)
{
	return val_cstr(&sym->val);
}

/*
 * Initializers
 */

#define __STATIC_CHAR_INITIALIZER(t, v)			\
	{						\
		.val = {				\
			.type = (t),			\
			.str_inline = { (v), '\0' },	\
			.static_struct = true,		\
			.static_alloc = true,		\
			.inline_alloc = true,		\
		}					\
	}
#define STR_STATIC_CHAR_INITIALIZER(v)			\
	__STATIC_CHAR_INITIALIZER(VT_STR, v)
#define SYM_STATIC_CHAR_INITIALIZER(v)			\
	__STATIC_CHAR_INITIALIZER(VT_SYM, v)

#define _STATIC_STR_INITIALIZER(t, v)			\
	{						\
		.val = {				\
			.type = (t),			\
			.str_ptr = (v),			\
			.static_struct = true,		\
			.static_alloc = true,		\
		}					\
	}
#define STR_STATIC_INITIALIZER(v)			\
		_STATIC_STR_INITIALIZER(VT_STR, (v))
#define SYM_STATIC_INITIALIZER(v)			\
		_STATIC_STR_INITIALIZER(VT_SYM, (v))

/* evaluates to a struct str *, so it can be used as a value */
#define STATIC_STR(s)					\
	({						\
		static struct str _s = STR_STATIC_INITIALIZER(s); \
		&_s;					\
	})

/* evaluates to a struct sym *, so it can be used as a value */
#define STATIC_SYM(s)					\
	({						\
		static struct sym _s = SYM_STATIC_INITIALIZER(s); \
		&_s;					\
	})

/*
 * Allocation wrappers.  These call the identically named functions and
 * assert that there was no error.
 */

#define _VAL_ALLOC(type, alloc)	\
	({				\
		type *_s;		\
		_s = alloc;		\
		ASSERT(!IS_ERR(_s));	\
		_s;			\
	})

#define VAL_ALLOC_CHAR(v)	_VAL_ALLOC(struct val, val_alloc_char(v))
#define VAL_ALLOC_INT(v)	_VAL_ALLOC(struct val, val_alloc_int(v))
#define VAL_ALLOC_BOOL(v)	val_alloc_bool(v) /* never fails */
#define VAL_ALLOC_NULL()	val_alloc_null() /* never fails */
#define VAL_ALLOC_CONS(h, t)	_VAL_ALLOC(struct val, val_alloc_cons((h), (t)))
#define VAL_ALLOC_NVL()		_VAL_ALLOC(struct val, val_alloc_nvl())

#define VAL_ALLOC_ARRAY(v, l)		_VAL_ALLOC(struct val, val_alloc_array((v), (l)))
#define VAL_ALLOC_ARRAY_DUP(v, l)	_VAL_ALLOC(struct val, val_alloc_array_dup((v), (l)))
#define VAL_ALLOC_ARRAY_STATIC(v, l)	_VAL_ALLOC(struct val, val_alloc_array_static((v), (l)))

#define STR_ALLOC(s)		_VAL_ALLOC(struct str, str_alloc(s))
#define SYM_ALLOC(s)		_VAL_ALLOC(struct sym, sym_alloc(s))
#define VAL_ALLOC_STR(s)	str_cast_to_val(STR_ALLOC(s))
#define VAL_ALLOC_SYM(s)	sym_cast_to_val(SYM_ALLOC(s))
#define STR_ALLOC_STATIC(s)	_VAL_ALLOC(struct str, str_alloc_static(s))
#define SYM_ALLOC_STATIC(s)	_VAL_ALLOC(struct sym, sym_alloc_static(s))
#define VAL_ALLOC_STR_STATIC(s)	str_cast_to_val(str_alloc_static(s))
#define VAL_ALLOC_SYM_STATIC(s)	sym_cast_to_val(sym_alloc_static(s))
#define STR_DUP(s)		_VAL_ALLOC(struct str, str_dup(s))
#define SYM_DUP(s)		_VAL_ALLOC(struct sym, sym_dup(s))
#define VAL_DUP_STR(s)		str_cast_to_val(str_dup(s))
#define VAL_DUP_SYM(s)		sym_cast_to_val(sym_dup(s))
#define STR_DUP_LEN(s, l)	_VAL_ALLOC(struct str, str_dup_len((s), (l)))
#define SYM_DUP_LEN(s, l)	_VAL_ALLOC(struct sym, sym_dup_len((s), (l)))
#define VAL_DUP_STR_LEN(s, l)	str_cast_to_val(str_dup_len((s), (l)))
#define VAL_DUP_SYM_LEN(s, l)	sym_cast_to_val(sym_dup_len((s), (l)))

#endif
