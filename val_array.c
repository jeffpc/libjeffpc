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

#include <jeffpc/mem.h>

#include "val_impl.h"

static struct val *__val_alloc_array(struct val **vals, size_t nelem,
				     bool dup, bool heap)
{
	struct val *val;
	size_t i;

	if (dup) {
		struct val **tmp;

		/* no reason to ever duplicate a heap allocated array */
		ASSERT(!heap);

		tmp = mem_reallocarray(NULL, nelem, sizeof(struct val *));
		if (!tmp) {
			val = ERR_PTR(-ENOMEM);
			goto err;
		}

		/*
		 * The multiplication can't overflow since the memory allocation
		 * already checks for it.
		 */
		memcpy(tmp, vals, sizeof(struct val *) * nelem);

		/* switch to the new array & free it on error or 0 refcnt */
		vals = tmp;
		heap = true;
	}

	val = __val_alloc(VT_ARRAY);
	if (IS_ERR(val))
		goto err;

	val->_set_array.vals = vals;
	val->_set_array.nelem = nelem;
	val->static_alloc = !heap;

	return val;

err:
	for (i = 0; i < nelem; i++)
		val_putref(vals[i]);

	if (heap)
		free(vals);

	return val;
}

struct val *val_alloc_array(struct val **vals, size_t nelem)
{
	return __val_alloc_array(vals, nelem, false, true);
}

struct val *val_alloc_array_dup(struct val **vals, size_t nelem)
{
	return __val_alloc_array(vals, nelem, true, false);
}

struct val *val_alloc_array_static(struct val **vals, size_t nelem)
{
	return __val_alloc_array(vals, nelem, false, false);
}
