/*
 * Copyright (c) 2017 Josef 'Jeff' Sipek <jeffpc@josefsipek.net>
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

#ifndef __JEFFPC_TASKQ_H
#define __JEFFPC_TASKQ_H

#include <jeffpc/thread.h>
#include <jeffpc/synch.h>

struct taskq_item {
	void (*fxn)(void *);
	void *arg;

	struct taskq_item *next;
};

struct taskq {
	char name[16];
	long nthreads;
	long nstarted_threads;
	pthread_t *threads;

	struct lock lock;
	struct cond cond_worker2parent;
	struct cond cond_parent2worker;

	struct taskq_item *first;
	struct taskq_item *last;
	unsigned long queue_len;

	bool shutdown;

	/* stats */
	unsigned long processed;
};

extern struct taskq *taskq_create_fixed(const char *name, long nthreads);
extern int taskq_dispatch(struct taskq *tq, void (*fxn)(void *), void *arg);
extern void taskq_wait(struct taskq *tq);
extern void taskq_destroy(struct taskq *tq);

#endif
