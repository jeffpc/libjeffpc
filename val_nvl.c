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

static struct mem_cache *nvpair_cache;

static void __attribute__((constructor)) init_val_subsys(void)
{
	nvpair_cache = mem_cache_create("nvpair-cache", sizeof(struct nvpair),
					0);
	ASSERT(!IS_ERR(nvpair_cache));
}

struct nvpair *__nvpair_alloc(const char *name)
{
	struct nvpair *pair;

	pair = mem_cache_alloc(nvpair_cache);
	if (!pair)
		return NULL;

	pair->name = str_dup(name);
	if (IS_ERR(pair->name)) {
		mem_cache_free(nvpair_cache, pair);
		return NULL;
	}

	/*
	 * Avoid returning with a NULL pointer.  Of all the types, VT_NULL
	 * is the least out of place.
	 *
	 * (We use VAL_ALLOC_NULL here to make it painfully clear that
	 * ->value is valid later on, even though we could have used
	 *  val_alloc_null directly as it never fails.)
	 */
	pair->value = VAL_ALLOC_NULL();

	return pair;
}

void __nvpair_free(struct nvpair *pair)
{
	str_putref(pair->name);
	val_putref(pair->value);

	mem_cache_free(nvpair_cache, pair);
}

static int val_nvl_cmp(const void *va, const void *vb)
{
	const struct nvpair *a = va;
	const struct nvpair *b = vb;

	return str_cmp(a->name, b->name);
}

struct val *val_alloc_nvl(void)
{
	struct val *val;

	val = __val_alloc(VT_NVL);
	if (IS_ERR(val))
		return val;

	bst_create(&val->_set_nvl.values, val_nvl_cmp, sizeof(struct nvpair),
		   offsetof(struct nvpair, node));

	return val;
}

void __val_free_nvl(struct val *val)
{
	struct bst_cookie cookie;
	struct nvpair *cur;

	ASSERT(val);
	ASSERT3U(refcnt_read(&val->refcnt), ==, 0);
	ASSERT3U(val->type, ==, VT_NVL);

	memset(&cookie, 0, sizeof(struct bst_cookie));
	while ((cur = bst_destroy_nodes(&val->_set_nvl.values, &cookie)))
		__nvpair_free(cur);

	bst_destroy(&val->_set_nvl.values);
}
