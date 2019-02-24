/*
 * Copyright (c) 2015-2019 Josef 'Jeff' Sipek <jeffpc@josefsipek.net>
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

#ifndef __JEFFPC_SEXPR_H
#define __JEFFPC_SEXPR_H

#include <stdbool.h>

#include <jeffpc/val.h>

struct sexpr_eval_env {
	struct val *(*symlookup)(struct sym *, struct sexpr_eval_env *);
	void *(*fxnlookup)(struct sym *, struct sexpr_eval_env *);
	void *private;
};

extern struct val *sexpr_parse(const char *str, size_t len);
extern struct str *sexpr_dump(struct val *lv, bool raw);
extern int sexpr_dump_file(FILE *out, struct val *lv, bool raw);
extern struct val *sexpr_array_to_list(struct val **vals, size_t nvals);
extern struct val *sexpr_args_to_list(size_t nvals, ...);
extern ssize_t sexpr_list_to_array(struct val *list, struct val **array,
				   size_t alen);
extern struct val *sexpr_list_to_val_array(struct val *list);

static inline struct val *sexpr_parse_str(struct str *str)
{
	if (!str)
		return ERR_PTR(-EINVAL);

	return sexpr_parse(str_cstr(str), str_len(str));
}

static inline struct val *sexpr_parse_cstr(const char *str)
{
	if (!str)
		return ERR_PTR(-EINVAL);

	return sexpr_parse(str, strlen(str));
}

extern struct val *sexpr_car(struct val *val);
extern struct val *sexpr_cdr(struct val *val);
/* get nth entry; 0 is the same val, 1 is the same a (car val) */
extern ssize_t sexpr_length(struct val *lv);
extern struct val *sexpr_nth(struct val *val, uint64_t n);
extern struct val *sexpr_assoc(struct val *val, const char *name);
extern bool sexpr_equal(struct val *lhs, struct val *rhs);

extern struct val *sexpr_eval(struct val *val, struct sexpr_eval_env *);

/*
 * Assorted helpers to make alists more pleasant to use.
 */
extern struct val *sexpr_alist_lookup_val(struct val *lv, const char *name);
extern struct str *sexpr_alist_lookup_str(struct val *lv, const char *name);
extern uint64_t sexpr_alist_lookup_int(struct val *lv, const char *name,
				       bool *found);
extern bool sexpr_alist_lookup_bool(struct val *lv, const char *name, bool def,
				    bool *found);
extern struct val *sexpr_alist_lookup_list(struct val *lv, const char *name);

static inline bool sexpr_is_null(struct val *v)
{
	return !v || (v->type == VT_CONS && !v->cons.head && !v->cons.tail);
}

/*
 * This is a helper, do not use it directly.  Instead use the sexpr_for_each
 * macros.
 */
static inline void __sexpr_for_each(struct val *curcons,
				    struct val *curtail,
				    struct val **nextval,
				    struct val **nextcons,
				    bool ref)
{
	if (sexpr_is_null(curtail) || (curtail->type != VT_CONS)) {
		*nextcons = NULL;
		*nextval  = NULL;
	} else {
		*nextcons = curtail;
		*nextval  = curtail->cons.head;
	}

	if (ref) {
		val_getref(*nextcons);
		val_putref(curcons);
	}
}

/*
 * Iterate over an s-expression list.
 *
 * sexpr_for_each consumes the passed in list, while sexpr_for_each_noref
 * assumes that the list will not disappear from under it.
 *
 * Caution: using 'break' inside sexpr_for_each will result in a reference
 * leak!
 */

#define sexpr_for_each(v, tmp, list) \
	for (__sexpr_for_each(list, list, &v, &tmp, true); \
	     v; \
	     __sexpr_for_each(tmp, tmp->cons.tail, &v, &tmp, true))

#define sexpr_for_each_noref(v, tmp, list) \
	for (__sexpr_for_each(list, list, &v, &tmp, false); \
	     v; \
	     __sexpr_for_each(tmp, tmp->cons.tail, &v, &tmp, false))

#endif
