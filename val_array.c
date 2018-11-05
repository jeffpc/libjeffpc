/*
 * Copyright (c) 2018 Josef 'Jeff' Sipek <jeffpc@josefsipek.net>
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
				     bool heap)
{
	struct val *val;

	val = __val_alloc(VT_ARRAY);
	if (IS_ERR(val)) {
		size_t i;

		for (i = 0; i < nelem; i++)
			val_putref(vals[i]);

		if (heap)
			free(vals);
	} else {
		val->_set_array.vals = vals;
		val->_set_array.nelem = nelem;
		val->static_alloc = !heap;
	}

	return val;
}

struct val *val_alloc_array(struct val **vals, size_t nelem)
{
	return __val_alloc_array(vals, nelem, true);
}

struct val *val_alloc_array_dup(struct val **vals, size_t nelem)
{
	struct val **tmp;

	tmp = mem_reallocarray(NULL, nelem, sizeof(struct val *));
	if (!tmp)
		return ERR_PTR(-ENOMEM);

	/*
	 * The multiplication can't overflow since the memory allocation
	 * already checks for it.
	 */
	memcpy(tmp, vals, sizeof(struct val *) * nelem);

	return __val_alloc_array(tmp, nelem, true);
}

struct val *val_alloc_array_static(struct val **vals, size_t nelem)
{
	return __val_alloc_array(vals, nelem, false);
}