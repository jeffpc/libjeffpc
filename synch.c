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

#include <stdio.h>
#include <stdlib.h>

#include <jeffpc/error.h>
#include <jeffpc/synch.h>
#include <jeffpc/config.h>

/*
 * held stack management
 */
#ifdef JEFFPC_LOCK_TRACKING
#define LOCKDEP_STACK_DEPTH	32

struct held_lock {
	struct lock *lock;
};

static __thread struct held_lock held_stack[LOCKDEP_STACK_DEPTH];
static __thread size_t held_stack_count;

#define for_each_held_lock(idx, cur)	\
	for (idx = 0, cur = &held_stack[0]; \
	     idx < held_stack_count; \
	     idx++, cur = &held_stack[idx])

static inline struct held_lock *last_acquired_lock(void)
{
	VERIFY3U(held_stack_count, >, 0);

	return &held_stack[held_stack_count - 1];
}

static inline struct held_lock *held_stack_alloc(void)
{
	if (held_stack_count == LOCKDEP_STACK_DEPTH)
		return NULL;

	return &held_stack[held_stack_count++];
}

static inline void held_stack_remove(struct held_lock *held)
{
	struct held_lock *last = last_acquired_lock();

	if (held != last)
		memmove(held, held + 1,
			(last - held) * sizeof(struct held_lock));

	held_stack_count--;
}

#endif

/*
 * error printing
 */
static void print_invalid_call(const char *fxn, const struct lock_context *where)
{
	cmn_err(CE_CRIT, "lockdep: invalid call to %s at %s:%d", fxn,
		where->file, where->line);
	panic("lockdep: Aborting.");
}

static void print_lock(struct lock *lock, const struct lock_context *where)
{
	cmn_err(CE_CRIT, "lockdep:     %s (%p) <%c> at %s:%d",
		"<unknown>",
		lock,
		(lock->magic != (uintptr_t) lock) ? 'M' : '.',
		where->file, where->line);
}

/*
 * state checking
 */
static void check_lock_magic(struct lock *lock, const char *op,
			     const struct lock_context *where)
{
	if (lock->magic == (uintptr_t) lock)
		return;

	cmn_err(CE_CRIT, "lockdep: thread trying to %s lock with bad magic", op);
	print_lock(lock, where);
	panic("lockdep: Aborting.");
}

static void verify_lock_init(const struct lock_context *where, struct lock *l,
			     const struct lock_class *lc)
{
	if (!l || !lc)
		print_invalid_call("MXINIT", where);

	l->magic = (uintptr_t) l;
}

static void verify_lock_destroy(const struct lock_context *where, struct lock *l)
{
	if (!l)
		print_invalid_call("MXDESTROY", where);

	check_lock_magic(l, "destroy", where);
}

static void verify_lock_lock(const struct lock_context *where, struct lock *l)
{
	if (!l)
		print_invalid_call("MXLOCK", where);

	check_lock_magic(l, "acquire", where);
}

static void verify_lock_unlock(const struct lock_context *where, struct lock *l)
{
	if (!l)
		print_invalid_call("MXUNLOCK", where);

	check_lock_magic(l, "release", where);
}

/*
 * synch API
 */
void mxinit(const struct lock_context *where, struct lock *l,
	    const struct lock_class *lc)
{
	verify_lock_init(where, l, lc);

	VERIFY0(pthread_mutex_init(&l->lock, NULL));
}

void mxdestroy(const struct lock_context *where, struct lock *l)
{
	verify_lock_destroy(where, l);

	VERIFY0(pthread_mutex_destroy(&l->lock));
}

void mxlock(const struct lock_context *where, struct lock *l)
{
	verify_lock_lock(where, l);

	VERIFY0(pthread_mutex_lock(&l->lock));
}

void mxunlock(const struct lock_context *where, struct lock *l)
{
	verify_lock_unlock(where, l);

	VERIFY0(pthread_mutex_unlock(&l->lock));
}

void rwinit(struct rwlock *l)
{
	VERIFY0(pthread_rwlock_init(&l->lock, NULL));
}

void rwdestroy(struct rwlock *l)
{
	VERIFY0(pthread_rwlock_destroy(&l->lock));
}

void rwlock(struct rwlock *l, bool wr)
{
	if (wr)
		VERIFY0(pthread_rwlock_wrlock(&l->lock));
	else
		VERIFY0(pthread_rwlock_rdlock(&l->lock));
}

void rwunlock(struct rwlock *l)
{
	VERIFY0(pthread_rwlock_unlock(&l->lock));
}

void condinit(struct cond *c)
{
	VERIFY0(pthread_cond_init(&c->cond, NULL));
}

void conddestroy(struct cond *c)
{
	VERIFY0(pthread_cond_destroy(&c->cond));
}

void condwait(struct cond *c, struct lock *l)
{
	VERIFY0(pthread_cond_wait(&c->cond, &l->lock));
}

int condreltimedwait(struct cond *c, struct lock *l,
		     const struct timespec *reltime)
{
	int ret;

#ifdef HAVE_PTHREAD_COND_RELTIMEDWAIT_NP
	ret = -pthread_cond_reltimedwait_np(&c->cond, &l->lock, reltime);
#else
	struct timespec abstime;
	struct timespec now;

	VERIFY0(clock_gettime(CLOCK_REALTIME, &now));

	while ((now.tv_nsec + reltime->tv_nsec) >= 1000000000) {
		now.tv_sec++;
		now.tv_nsec -= 1000000000;
	}

	abstime.tv_sec  = now.tv_sec  + reltime->tv_sec;
	abstime.tv_nsec = now.tv_nsec + reltime->tv_nsec;

	ret = -pthread_cond_timedwait(&c->cond, &l->lock, &abstime);
#endif

	if ((ret != 0) && (ret != -ETIMEDOUT))
		panic("%s failed: %s", __func__, xstrerror(ret));

	return ret;
}

void condsig(struct cond *c)
{
	VERIFY0(pthread_cond_signal(&c->cond));
}

void condbcast(struct cond *c)
{
	VERIFY0(pthread_cond_broadcast(&c->cond));
}

void barrierinit(struct barrier *b, unsigned count)
{
	VERIFY0(pthread_barrier_init(&b->bar, NULL, count));
}

void barrierdestroy(struct barrier *b)
{
	VERIFY0(pthread_barrier_destroy(&b->bar));
}

bool barrierwait(struct barrier *b)
{
	int ret;

	ret = pthread_barrier_wait(&b->bar);

	VERIFY((ret == 0) || (ret == PTHREAD_BARRIER_SERIAL_THREAD));

	return (ret == PTHREAD_BARRIER_SERIAL_THREAD);
}
