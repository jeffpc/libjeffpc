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

#include <jeffpc/synch.h>

#include "test.c"

#define NLOCKS	(JEFFPC_LOCK_STACK_DEPTH + 1)

void test(void)
{
	struct lock_class lc[NLOCKS];
	struct rwlock lock[NLOCKS];
	size_t i;

	for (i = 0; i < NLOCKS; i++) {
		char *name;

		/* initialize the lock class */
		asprintf(&name, "class %zu", i);
		memset(&lc[i], 0, sizeof(struct lock_class));
		lc[i].name = name;

		/* initialize the lock */
		RWINIT(&lock[i], &lc[i]);
	}

	/* lock enough to fill up the stack */
	for (i = 0; i < (NLOCKS - 1); i++)
		RWLOCK(&lock[i], false);

	/* lock another lock to overflow the stack */
	RWLOCK(&lock[NLOCKS - 1], false);

	/* TODO: check lockdep state */
}
