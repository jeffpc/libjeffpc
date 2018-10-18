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

#ifndef __JEFFPC_SYNCH_H
#define __JEFFPC_SYNCH_H

#include <stdbool.h>
#include <stdint.h>
#include <pthread.h>

#include <jeffpc/config-synch.h>

struct lock_class {
	const char *name;
#ifdef JEFFPC_LOCK_TRACKING
	size_t ndeps;
	struct lock_class *deps[JEFFPC_LOCK_DEP_COUNT];
#endif
};

#define LOCK_CLASS(n)	struct lock_class n = { .name = #n };

struct lock_context {
	const char *lockname;
	const char *file;
	int line;
};

struct lock {
	pthread_mutex_t lock;
	uintptr_t magic;
#ifdef JEFFPC_LOCK_TRACKING
	struct lock_class *lc;
	const char *name;
#endif
};

struct rwlock {
	pthread_rwlock_t lock;
};

struct cond {
	pthread_cond_t cond;
};

struct barrier {
	pthread_barrier_t bar;
};

/* the API */
#define MXINIT(l, lc)	do { \
				struct lock_context mx_ctx = { \
					.lockname = #l, \
					.file = __FILE__, \
					.line = __LINE__, \
				}; \
				mxinit(&mx_ctx, (l), (lc)); \
			 } while (0)
#define MXDESTROY(l)	do { \
				struct lock_context mx_ctx = { \
					.lockname = #l, \
					.file = __FILE__, \
					.line = __LINE__, \
				}; \
				mxdestroy(&mx_ctx, (l)); \
			} while (0)
#define MXLOCK(l)	do { \
				struct lock_context mx_ctx = { \
					.lockname = #l, \
					.file = __FILE__, \
					.line = __LINE__, \
				}; \
				mxlock(&mx_ctx, (l)); \
			} while (0)
#define MXUNLOCK(l)	do { \
				struct lock_context mx_ctx = { \
					.lockname = #l, \
					.file = __FILE__, \
					.line = __LINE__, \
				}; \
				mxunlock(&mx_ctx, (l)); \
			} while (0)
#define CONDINIT(c)	condinit(c)
#define CONDDESTROY(c)	conddestroy(c)
#define CONDWAIT(c,m)	condwait((c),(m))
#define CONDSIG(c)	condsig(c)
#define CONDBCAST(c)	condbcast(c)

/* Do *NOT* use directly */
extern void mxinit(const struct lock_context *where, struct lock *m,
		   struct lock_class *lc);
extern void mxdestroy(const struct lock_context *where, struct lock *m);
extern void mxlock(const struct lock_context *where, struct lock *m);
extern void mxunlock(const struct lock_context *where, struct lock *m);

extern void rwinit(struct rwlock *l);
extern void rwdestroy(struct rwlock *l);
extern void rwlock(struct rwlock *l, bool wr);
extern void rwunlock(struct rwlock *l);

extern void condinit(struct cond *c);
extern void conddestroy(struct cond *c);
extern void condwait(struct cond *c, struct lock *m);
extern int condreltimedwait(struct cond *c, struct lock *m,
			    const struct timespec *reltime);
extern void condsig(struct cond *c);
extern void condbcast(struct cond *c);

extern void barrierinit(struct barrier *b, unsigned count);
extern void barrierdestroy(struct barrier *b);
extern bool barrierwait(struct barrier *b);

#endif
