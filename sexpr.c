/*
 * Copyright (c) 2015-2018 Josef 'Jeff' Sipek <jeffpc@josefsipek.net>
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

#include <stdlib.h>
#include <string.h>

#include <jeffpc/sexpr.h>
#include <jeffpc/mem.h>

#include "sexpr_impl.h"

struct val *sexpr_parse(const char *str, size_t len)
{
	struct sexpr_parser_state x;
	int ret;

	x.input = str;
	x.len   = len;
	x.pos   = 0;

	sexpr_reader_lex_init(&x.scanner);
	sexpr_reader_set_extra(&x, x.scanner);

	ret = sexpr_reader_parse(&x);

	sexpr_reader_lex_destroy(x.scanner);

	return ret ? ERR_PTR(-EINVAL) : x.output;
}

/*
 * Convert a C array of vals into a sexpr list.  E.g.,
 *
 *	vals = { A, B, C }, nvals = 3
 *
 * turns into:
 *
 *	'(A . (B . (C . ())))
 *
 * which is the same as:
 *
 *	'(A B C)
 */
struct val *sexpr_array_to_list(struct val **vals, size_t nvals)
{
	struct val *last = NULL;
	struct val *tmp;

	if (!nvals)
		return val_empty_cons();

	for (; nvals > 0; nvals--, last = tmp)
		tmp = VAL_ALLOC_CONS(vals[nvals - 1], last);

	return last;
}

/*
 * Just like sexpr_array_to_list, but obtains value from args instead of an
 * array.
 */
struct val *sexpr_args_to_list(size_t nvals, ...)
{
	const size_t n = nvals;
	struct val *arr[n];
	va_list args;
	size_t i;

	va_start(args, nvals);
	for (i = 0; i < n; i++)
		arr[i] = va_arg(args, struct val *);
	va_end(args);

	return sexpr_array_to_list(arr, n);
}

/*
 * Convert a sexpr list into a C array of vals.  E.g.,
 *
 *     '(A B C)
 *
 * turns into:
 *
 *     array = { A, B, C }, nvals = 3
 *
 * We fill in the passed in array with at most alen elements.  The number of
 * filled in elements is returned to the caller.
 */
int sexpr_list_to_array(struct val *list, struct val **array, int alen)
{
	struct val *tmp;
	int nvals = 0;

	for (tmp = list;
	     !sexpr_is_null(tmp) && (alen > nvals);
	     tmp = tmp->cons.tail, nvals++) {
		if (tmp->type != VT_CONS)
			goto err;

		array[nvals] = val_getref(tmp->cons.head);
	}

	if ((alen == nvals) && !sexpr_is_null(tmp))
		goto err;

	return nvals;

err:
	while (nvals)
		val_putref(array[--nvals]);

	return -1;
}

/*
 * Convert a sexpr list into a C array of vals.  E.g.,
 *
 *     '(A B C)
 *
 * turns into a VT_ARRAY containing:
 *
 *     { A, B, C }
 */
struct val *sexpr_list_to_val_array(struct val *list)
{
	struct val **arr;
	ssize_t len;
	int ret;

	len = sexpr_length(val_getref(list));
	if (len < 0) {
		val_putref(list);
		return ERR_PTR(-EINVAL);
	}

	arr = mem_reallocarray(NULL, len, sizeof(struct val *));
	if (!arr) {
		val_putref(list);
		return ERR_PTR(-ENOMEM);
	}

	ret = sexpr_list_to_array(list, arr, len);
	if (ret != len) {
		free(arr);
		return ERR_PTR(-EINVAL);
	}

	return val_alloc_array(arr, len);
}

struct val *sexpr_car(struct val *lv)
{
	struct val *ret;

	if (!lv)
		return NULL;

	if (lv->type == VT_CONS)
		ret = val_getref(lv->cons.head);
	else
		ret = NULL;

	val_putref(lv);

	return ret;
}

struct val *sexpr_cdr(struct val *lv)
{
	struct val *ret;

	if (!lv)
		return NULL;

	if (lv->type == VT_CONS)
		ret = val_getref(lv->cons.tail);
	else
		ret =  NULL;

	val_putref(lv);

	return ret;
}

ssize_t sexpr_length(struct val *lv)
{
	ssize_t len;

	len = 0;

	while (!sexpr_is_null(lv)) {
		if (lv->type != VT_CONS) {
			/* not a list */
			val_putref(lv);
			return -1;
		}

		len++;
		lv = sexpr_cdr(lv);
	}

	val_putref(lv);

	return len;
}

struct val *sexpr_nth(struct val *lv, uint64_t n)
{
	while (n-- && lv) {
		struct val *tmp;

		if (lv->type == VT_CONS) {
			/*
			 * If this is not the one we want, follow the tail.
			 * Otherwise, grab the head.
			 */
			if (n)
				tmp = val_getref(lv->cons.tail);
			else
				tmp = val_getref(lv->cons.head);
		} else {
			tmp = NULL;
		}

		val_putref(lv);

		lv = tmp;
	}

	return lv;
}

/*
 * Given a list, lookup a certain name.
 *
 * The input list looks like:
 *   '((a . b) (c . d))
 * which really is:
 *   '((a . b) . ((c . d) . ()))
 *
 * So, to check it, we examine the car of the list, if that's not the right
 * key, we recurse on cdr of the list.
 */
struct val *sexpr_assoc(struct val *lv, const char *name)
{
	struct val *head;
	struct val *tail;

	/* empty list */
	if (!lv)
		return NULL;

	/* not a list */
	if (lv->type != VT_CONS)
		return NULL;

	head = lv->cons.head;
	tail = lv->cons.tail;

	/*
	 * check the head of current cons cell: '(head . tail)
	 *   (1) must be non-null
	 *   (2) must be a cons cell, i.e.,  head == '(a . b)
	 *   (3) (car head) must be a string or symbol
	 *   (4) (car head) must be equal to the value passed in
	 */
	if (head && (head->type == VT_CONS) &&
	    head->cons.head &&
	    ((head->cons.head->type == VT_STR) ||
	     (head->cons.head->type == VT_SYM)) &&
	    !strcmp(val_cstr(head->cons.head), name))
		return val_getref(head);

	return sexpr_assoc(tail, name);
}

bool sexpr_equal(struct val *lhs, struct val *rhs)
{
	bool ret;

	/* if they are the same object, they are equal - even if NULL */
	if (lhs == rhs) {
		ret = true;
		goto out;
	}

	/* if one is NULL, they are unequal */
	if (!lhs || !rhs) {
		/* ... unless we're comparing a NULL with a '() */
		if ((!lhs && ((rhs->type != VT_CONS) ||
			      rhs->cons.head ||
			      rhs->cons.tail)) ||
		    (!rhs && ((lhs->type != VT_CONS) ||
			      lhs->cons.head ||
			      lhs->cons.tail))) {
			ret = false;
			goto out;
		}

		ret = true;
		goto out;
	}

	/*
	 * At this point, we have two non-NULL values.
	 */

	/* different type -> unequal */
	if (lhs->type != rhs->type) {
		ret = false;
		goto out;
	}

	switch (lhs->type) {
		case VT_NULL:
			ret = true;
			goto out;
		case VT_INT:
		case VT_CHAR:
			ret = (lhs->i == rhs->i);
			goto out;
		case VT_STR:
		case VT_SYM:
			ret = str_cmp(val_cast_to_str(lhs),
				      val_cast_to_str(rhs)) == 0;
			goto out;
		case VT_BOOL:
			ret = (lhs->b == rhs->b);
			goto out;
		case VT_BLOB:
			ret = (lhs->blob.size == rhs->blob.size);
			if (!ret)
				goto out;

			ret = (lhs->blob.ptr == rhs->blob.ptr);
			if (ret)
				goto out;

			ret = (memcmp(lhs->blob.ptr, rhs->blob.ptr,
				      lhs->blob.size) == 0);
			goto out;
		case VT_CONS:
			ret = sexpr_equal(val_getref(lhs->cons.head),
					  val_getref(rhs->cons.head)) &&
			      sexpr_equal(val_getref(lhs->cons.tail),
					  val_getref(rhs->cons.tail));
			goto out;
		case VT_ARRAY: {
			size_t i;

			ret = (lhs->array.nelem == rhs->array.nelem);
			if (!ret)
				goto out;

			for (i = 0; i < lhs->array.nelem; i++) {
				ret = sexpr_equal(val_getref(lhs->_set_array.vals[i]),
						  val_getref(rhs->_set_array.vals[i]));
				if (!ret)
					break;
			}

			goto out;
		}
		case VT_NVL: {
			struct bst_tree *ltree = &lhs->_set_nvl.values;
			struct bst_tree *rtree = &rhs->_set_nvl.values;
			struct nvpair *lcur;
			struct nvpair *rcur;

			lcur = bst_first(ltree);
			rcur = bst_first(rtree);

			while (lcur && rcur) {
				ret = (str_cmp(lcur->name, rcur->name) == 0);
				if (!ret)
					goto out;

				ret = sexpr_equal(val_getref(lcur->value),
						  val_getref(rcur->value));
				if (!ret)
					goto out;

				lcur = bst_next(ltree, lcur);
				rcur = bst_next(rtree, rcur);
			}

			/* if both sides reached the end, then they are equal */
			ret = !lcur && !rcur;
			goto out;
		}
	}

	panic("unknown struct val type %u", lhs->type);

out:
	val_putref(lhs);
	val_putref(rhs);
	return ret;
}

struct val *sexpr_alist_lookup_val(struct val *lv, const char *name)
{
	if (!lv || !name)
		return NULL;

	return sexpr_cdr(sexpr_assoc(lv, name));
}

struct str *sexpr_alist_lookup_str(struct val *lv, const char *name)
{
	struct str *ret;
	struct val *v;

	if (!lv || !name)
		return NULL;

	v = sexpr_cdr(sexpr_assoc(lv, name));
	if (!v || (v->type != VT_STR))
		ret = NULL;
	else
		ret = val_cast_to_str(val_getref(v));

	val_putref(v);

	return ret;
}

uint64_t sexpr_alist_lookup_int(struct val *lv, const char *name, bool *found)
{
	struct val *v;
	uint64_t ret;
	bool ok;

	if (!lv || !name) {
		if (found)
			*found = false;
		return 0;
	}

	v = sexpr_cdr(sexpr_assoc(lv, name));
	ok = v && (v->type == VT_INT);

	if (!ok)
		ret = 0;
	else
		ret = v->i;

	val_putref(v);

	if (found)
		*found = ok;

	return ret;
}

bool sexpr_alist_lookup_bool(struct val *lv, const char *name, bool def,
			     bool *found)
{
	struct val *v;
	bool ret;
	bool ok;

	if (!lv || !name) {
		if (found)
			*found = false;
		return def;
	}

	v = sexpr_cdr(sexpr_assoc(lv, name));
	ok = v && (v->type == VT_BOOL);

	if (!ok)
		ret = def;
	else
		ret = v->b;

	val_putref(v);

	if (found)
		*found = ok;

	return ret;
}

struct val *sexpr_alist_lookup_list(struct val *lv, const char *name)
{
	struct val *ret;
	struct val *v;

	if (!lv || !name)
		return NULL;

	v = sexpr_cdr(sexpr_assoc(lv, name));
	if (!v || (v->type != VT_CONS))
		ret = NULL;
	else
		ret = val_getref(v);

	val_putref(v);

	return ret;
}
