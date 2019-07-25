/*
 * Copyright (c) 2011-2019 Josef 'Jeff' Sipek <jeffpc@josefsipek.net>
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

#include <jeffpc/config.h>

struct lock_class {
	const char *name;
#ifdef JEFFPC_LOCK_TRACKING
	size_t ndeps;
	struct lock_class *deps[JEFFPC_LOCK_DEP_COUNT];
#endif
};

#define LOCK_CLASS(n)	struct lock_class n = { .name = #n };

struct lock_context {
	union {
		/* mutex/cond/rwlock */
		struct {
			const char *condname; /* cond */
			const char *lockname; /* mutex or rwlock */
		};
		/* barrier */
		const char *barname; /* barrier */
	};
	const char *file;
	int line;
};

struct lock_info {
	uintptr_t magic;
	unsigned int type;
};

struct lock {
	struct lock_info info;
	pthread_mutex_t lock;
#ifdef JEFFPC_LOCK_TRACKING
	struct lock_class *lc;
	const char *name;
#endif
};

struct rwlock {
	struct lock_info info;
	pthread_rwlock_t lock;
};

struct cond {
	struct lock_info info;
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
#define RWINIT(l)	do { \
				struct lock_context rw_ctx = { \
					.lockname = #l, \
					.file = __FILE__, \
					.line = __LINE__, \
				}; \
				rwinit(&rw_ctx, (l)); \
			} while (0)
#define RWDESTROY(l)	do { \
				struct lock_context rw_ctx = { \
					.lockname = #l, \
					.file = __FILE__, \
					.line = __LINE__, \
				}; \
				rwdestroy(&rw_ctx, (l)); \
			} while (0)
#define RWLOCK(l, wr)	do { \
				struct lock_context rw_ctx = { \
					.lockname = #l, \
					.file = __FILE__, \
					.line = __LINE__, \
				}; \
				rwlock(&rw_ctx, (l), (wr)); \
			} while (0)
#define RWUNLOCK(l)	do { \
				struct lock_context rw_ctx = { \
					.lockname = #l, \
					.file = __FILE__, \
					.line = __LINE__, \
				}; \
				rwunlock(&rw_ctx, (l)); \
			} while (0)
#define CONDINIT(c)	do { \
				struct lock_context cond_ctx = { \
					.condname = #c, \
					.file = __FILE__, \
					.line = __LINE__, \
				}; \
				condinit(&cond_ctx, (c)); \
			} while (0)
#define CONDDESTROY(c)	do { \
				struct lock_context cond_ctx = { \
					.condname = #c, \
					.file = __FILE__, \
					.line = __LINE__, \
				}; \
				conddestroy(&cond_ctx, (c)); \
			} while (0)
#define CONDWAIT(c, m)	do { \
				struct lock_context cond_ctx = { \
					.condname = #c, \
					.lockname = #m, \
					.file = __FILE__, \
					.line = __LINE__, \
				}; \
				condwait(&cond_ctx, (c), (m)); \
			} while (0)
#define CONDTIMEDWAIT_NSEC(c, m, t) \
			do { \
				struct lock_context cond_ctx = { \
					.condname = #c, \
					.lockname = #m, \
					.file = __FILE__, \
					.line = __LINE__, \
				}; \
				condtimedwait(&cond_ctx, (c), (m), (t)); \
			} while (0)
#define CONDTIMEDWAIT_SEC(c, m, t) \
			CONDTIMEDWAIT_NSEC((c), (m), (t) * 1000000000ull)
#define CONDTIMEDWAIT_SPEC(c, m, t) \
			do { \
				struct time_spec tmp = *(t); \
				CONDTIMEDWAIT_NSEC((c), (m), \
						   (tmp.tv_sec * 1000000000ull) + \
						   tmp->tv_nsec); \
			} while (0)
#define CONDSIG(c)	do { \
				struct lock_context cond_ctx = { \
					.condname = #c, \
					.file = __FILE__, \
					.line = __LINE__, \
				}; \
				condsig(&cond_ctx, (c)); \
			} while (0)
#define CONDBCAST(c)	do { \
				struct lock_context cond_ctx = { \
					.condname = #c, \
					.file = __FILE__, \
					.line = __LINE__, \
				}; \
				condbcast(&cond_ctx, (c)); \
			} while (0)
#define BARRIERINIT(b, c) \
			do { \
				struct lock_context bar_ctx = { \
					.barname = #b, \
					.file = __FILE__, \
					.line = __LINE__, \
				}; \
				barrierinit(&bar_ctx, (b), (c)); \
			} while (0)
#define BARRIERDESTROY(b) \
			do { \
				struct lock_context bar_ctx = { \
					.barname = #b, \
					.file = __FILE__, \
					.line = __LINE__, \
				}; \
				barrierdestroy(&bar_ctx, (b)); \
			} while (0)
#define BARRIERWAIT(b)	do { \
				struct lock_context bar_ctx = { \
					.barname = #b, \
					.file = __FILE__, \
					.line = __LINE__, \
				}; \
				barrierwait(&bar_ctx, (b)); \
			} while (0)

/* assert that this thread is not holding any locks */
#ifdef JEFFPC_LOCK_TRACKING
extern void lockdep_no_locks(void);
#else
#define lockdep_no_locks()	do { } while (0)
#endif

/* Do *NOT* use directly */
extern void mxinit(const struct lock_context *where, struct lock *m,
		   struct lock_class *lc);
extern void mxdestroy(const struct lock_context *where, struct lock *m);
extern void mxlock(const struct lock_context *where, struct lock *m);
extern void mxunlock(const struct lock_context *where, struct lock *m);

extern void rwinit(const struct lock_context *where, struct rwlock *l);
extern void rwdestroy(const struct lock_context *where, struct rwlock *l);
extern void rwlock(const struct lock_context *where, struct rwlock *l, bool wr);
extern void rwunlock(const struct lock_context *where, struct rwlock *l);

extern void condinit(const struct lock_context *where, struct cond *c);
extern void conddestroy(const struct lock_context *where, struct cond *c);
extern void condwait(const struct lock_context *where, struct cond *c,
		     struct lock *m);
extern int condtimedwait(const struct lock_context *where, struct cond *c,
			 struct lock *m, const uint64_t reltime);
extern void condsig(const struct lock_context *where, struct cond *c);
extern void condbcast(const struct lock_context *where, struct cond *c);

extern void barrierinit(const struct lock_context *where, struct barrier *b,
			unsigned count);
extern void barrierdestroy(const struct lock_context *where, struct barrier *b);
extern bool barrierwait(const struct lock_context *where, struct barrier *b);

#endif
