/*
 * Copyright (c) 2018-2019 Josef 'Jeff' Sipek <jeffpc@josefsipek.net>
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

#include <jeffpc/sexpr.h>
#include <jeffpc/nvl.h>
#include <jeffpc/mem.h>

#include "sexpr_impl.h"

/*
 * Compact an array.
 *
 * Iterate over all the elements converting each.  If at the end, we end up
 * with the same array we free the new one we made, and just return the
 * old.
 *
 * Note: Under no circumstance can we modify the array in-place.  It may
 * have multiple references and we do not wish to modify all of them - only
 * the one that was passed in.
 */
static struct val *sexpr_compact_array(struct val *in)
{
	struct val **vals;
	size_t nelem;
	bool equal;
	size_t i;

	VERIFY3U(in->type, ==, VT_ARRAY);

	nelem = in->array.nelem;

	vals = alloca(sizeof(struct val *) * nelem);

	equal = true;

	for (i = 0; i < nelem; i++) {
		vals[i] = sexpr_compact(val_getref(in->array.vals[i]));

		/*
		 * We compare pointers on purpose.  This checks whether or
		 * not the sexpr_compact call above was a no-op.
		 */
		equal = equal && (vals[i] == in->array.vals[i]);
	}

	if (equal) {
		for (i = 0; i < nelem; i++)
			val_putref(vals[i]);

		return in;
	} else {
		val_putref(in);

		return val_alloc_array_dup(vals, nelem);
	}
}

/*
 * Compact an nvlist.
 *
 * Iterate over all the nv pairs converting each.  If at the end, we end up
 * with the same nvlist we free the new one we made, and just return the
 * old.
 *
 * Note: Under no circumstance can we modify the nvlist in-place.  It may
 * have multiple references and we do not wish to modify all of them - only
 * the one that was passed in.
 */
static struct val *sexpr_compact_nvl(struct val *in)
{
	const struct nvpair *pair;
	struct nvlist *src, *dst;
	struct val *ret_ptr;
	bool equal;

	VERIFY3U(in->type, ==, VT_NVL);

	src = val_cast_to_nvl(in);

	dst = nvl_alloc();
	if (IS_ERR(dst)) {
		ret_ptr = ERR_CAST(dst);
		goto err;
	}

	equal = true;

	nvl_for_each(pair, src) {
		struct nvpair tmp;
		int ret;

		tmp.name = pair->name;
		tmp.value = sexpr_compact(val_getref(pair->value));
		if (IS_ERR(tmp.value)) {
			ret_ptr = tmp.value;
			goto err_dst;
		}

		/*
		 * We compare pointers on purpose.  This checks whether or
		 * not the sexpr_compact call above was a no-op.
		 */
		equal = equal && (tmp.value == pair->value);

		/* takes a new ref on both name & val */
		ret = nvl_set_pair(dst, &tmp);

		val_putref(tmp.value); /* release our ref */

		if (ret) {
			ret_ptr = ERR_PTR(ret);
			goto err_dst;
		}
	}

	if (equal) {
		nvl_putref(dst);
		return in;
	} else {
		nvl_putref(src);
		return nvl_cast_to_val(dst);
	}

err_dst:
	nvl_putref(dst);
err:
	val_putref(in);
	return ret_ptr;
}

/*
 * Compact a cons list into an array.
 *
 * We assert a number of return values because the calls should never fail.
 * A failure means that our what-is-it logic is incorrect.  In that case, it
 * is better to assert than to incorrectly mangle the data.
 */
static struct val *sexpr_compact_cons_array(struct val *in)
{
	struct val **arr;
	ssize_t len;
	size_t i;
	int ret;

	len = sexpr_length(val_getref(in));

	/* assert, to catch issues with what-is-it logic */
	ASSERT3S(len, >=, 0);

	arr = alloca(sizeof(struct val *) * len);

	ret = sexpr_list_to_array(in, arr, len);

	/* assert, to catch issues with what-is-it logic */
	ASSERT3S(ret, ==, len);

	/* compact each element */
	for (i = 0; i < len; i++) {
		arr[i] = sexpr_compact(arr[i]);
		if (IS_ERR(arr[i])) {
			ret = PTR_ERR(arr[i]);
			goto err;
		}
	}

	/* make a new VT_ARRAY with the compacted values */
	return val_alloc_array_dup(arr, len);

err:
	for (i = 0; i < len; i++) {
		if (IS_ERR(arr[i]))
			continue;

		val_putref(arr[i]);
	}

	return ERR_PTR(ret);
}

/*
 * Compact a cons alist into an nvlist.
 */
static struct val *sexpr_compact_cons_nvl(struct val *in)
{
	struct val *cur, *tmp;
	struct nvlist *out;
	int ret;

	out = nvl_alloc();
	if (IS_ERR(out)) {
		val_putref(in);
		return nvl_cast_to_val(out);
	}

	/*
	 * compact each element
	 *
	 * For example:
	 *
	 *   ((a . b) (c . d))
	 *
	 * will cause iterations (cur):
	 *
	 *   (a . b)
	 *   (c . d)
	 */
	sexpr_for_each_noref(cur, tmp, in) {
		struct val *name = cur->cons.head;
		struct val *value = cur->cons.tail;
		struct nvpair new;

		if (name->type == VT_STR)
			new.name = val_getref_str(name);
		else
			new.name = str_dup(val_cstr(name));

		new.value = sexpr_compact(val_getref(value));
		if (IS_ERR(new.value)) {
			str_putref(new.name);
			ret = PTR_ERR(new.value);
			goto err;
		}

		/* takes a new ref on both name & value */
		ret = nvl_set_pair(out, &new);

		/* release our refs */
		str_putref(new.name);
		val_putref(new.value);

		if (ret)
			goto err;
	}

	val_putref(in);

	return nvl_cast_to_val(out);

err:
	val_putref(in);
	nvl_putref(out);

	return ERR_PTR(ret);
}


/*
 * Tries to figure out what sort of structure the passed in cons cell
 * represents.
 *
 * Returns:
 *    VT_NULL  - empty cons (technically convertible to either nvlist or array)
 *    VT_NVL   - an alist that's convertible to an nvlist
 *    VT_ARRAY - a list that's convertible to an array
 *    VT_CONS  - no conversion can be made
 */
static enum val_type what_is_it(struct val *in)
{
	struct val *cur;
	int nvl;

	if (sexpr_is_null(in))
		return VT_NULL;

	nvl = true;

	/*
	 * Iterate over the cons list checking each cell for whether it is a
	 * part of a list (i.e., cdr is cons) and whether it is a part of an
	 * alist (i.e., car is a cons with a string key)
	 *
	 * We don't use sexpr_for_each here because it stops iteration of
	 * non-lists early.
	 */
	for (cur = in; !sexpr_is_null(cur); cur = cur->cons.tail) {
		struct val *item;

		if (cur->type != VT_CONS)
			return VT_CONS;

		/* a cons cell = we have a list */

		/*
		 * Don't bother checking for nvl-ness if we already found at
		 * least one issue earlier.
		 */
		if (!nvl)
			continue;

		item = cur->cons.head;

		/* Is the current item a name-value cons? */

		/* ...is it a cons? */
		if (sexpr_is_null(item) || (item->type != VT_CONS)) {
			nvl = false;
			continue;
		}

		/* ...does it have a string/symbol name? */
		if (sexpr_is_null(item->cons.head) ||
		    ((item->cons.head->type != VT_STR) &&
		     (item->cons.head->type != VT_SYM))) {
			nvl = false;
			continue;
		}

		/* looks like an nvlist pair, keep going */
	}

	return nvl ? VT_NVL : VT_ARRAY;
}

static struct val *sexpr_compact_cons(struct val *in)
{
	VERIFY3U(in->type, ==, VT_CONS);

	switch (what_is_it(in)) {
		case VT_CONS:
			/* no conversion possible */
			return in;
		case VT_NULL:
			/* an empty cons; leave it as-is */
			return in;
		case VT_ARRAY:
			return sexpr_compact_cons_array(in);
		case VT_NVL:
			return sexpr_compact_cons_nvl(in);
		case VT_INT:
		case VT_STR:
		case VT_SYM:
		case VT_BOOL:
		case VT_CHAR:
		case VT_BLOB:
			break;
	}

	panic("impossible cons compaction target");
}

struct val *sexpr_compact(struct val *in)
{
	switch (in->type) {
		case VT_NULL:
		case VT_INT:
		case VT_BOOL:
		case VT_STR:
		case VT_SYM:
		case VT_CHAR:
		case VT_BLOB:
			/* nothing to do */
			return in;
		case VT_ARRAY:
			return sexpr_compact_array(in);
		case VT_NVL:
			return sexpr_compact_nvl(in);
		case VT_CONS:
			return sexpr_compact_cons(in);
	}

	panic("Cannot compact unknown val type (%d)", in->type);
}
