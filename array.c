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

#include <string.h>

#include <jeffpc/array.h>
#include <jeffpc/error.h>

struct array {
	size_t elem_size;		/* size of each element */
	size_t elem_count;		/* number of visible elements */
	size_t preallocated;		/* number of allocated elements */
	uint8_t raw[];
};

static inline size_t total_size(size_t elem_size, size_t count)
{
	return sizeof(struct array) + elem_size * count;
}

static inline void *get_ptr(struct array *array, size_t idx)
{
	return &array->raw[array->elem_size * idx];
}

void *array_alloc(size_t elem_size, size_t prealloc_count)
{
	struct array *array;

	array = malloc(total_size(elem_size, prealloc_count));
	if (!array)
		return NULL;

	array->elem_size = elem_size;
	array->elem_count = 0;
	array->preallocated = prealloc_count;

	return array->raw;
}

void array_free(void *raw)
{
	if (!raw)
		return;

	free(container_of(raw, struct array, raw));
}

static struct array *__array_truncate(struct array *array,
				      size_t new_elem_count)
{
	struct array *newarray;

	if (new_elem_count <= array->elem_count)
		goto done; /* shrinking or no change */

	if (new_elem_count <= array->preallocated)
		goto clear; /* growing into preallocated space */

	/* grow the allocation */

	newarray = realloc(array, total_size(array->elem_size, new_elem_count));
	if (!newarray)
		return ERR_PTR(-ENOMEM);

	array = newarray;

	array->preallocated = new_elem_count;

clear:
	memset(get_ptr(array, array->elem_count), 0,
	       total_size(array->elem_size, new_elem_count) -
	       total_size(array->elem_size, array->elem_count));

done:
	array->elem_count = new_elem_count;

	return array;
}

#undef array_truncate
int array_truncate(void **raw, size_t idx)
{
	struct array *array = container_of(*raw, struct array, raw);

	array = __array_truncate(array, idx);
	if (IS_ERR(array))
		return PTR_ERR(array);

	*raw = array->raw;

	return 0;
}

size_t array_size(void *raw)
{
	if (!raw)
		return 0;

	return container_of(raw, struct array, raw)->elem_count;
}
