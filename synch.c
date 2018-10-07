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
#include <jeffpc/atomic.h>
#include <jeffpc/synch.h>
#include <jeffpc/config.h>

#ifdef JEFFPC_LOCK_TRACKING
static atomic_t lockdep_on = ATOMIC_INITIALIZER(1);
static pthread_mutex_t lockdep_lock = PTHREAD_MUTEX_INITIALIZER;
#endif

/*
 * held stack management
 */
#ifdef JEFFPC_LOCK_TRACKING

struct held_lock {
	struct lock *lock;
	struct lock_context where;
};

static __thread struct held_lock held_stack[JEFFPC_LOCK_STACK_DEPTH];
static __thread size_t held_stack_count;

#define for_each_held_lock(idx, cur)	\
	for (idx = 0, cur = &held_stack[0]; \
	     idx < held_stack_count; \
	     idx++, cur = &held_stack[idx])

static inline struct held_lock *last_acquired_lock(void)
{
	if (!held_stack_count)
		return NULL;

	return &held_stack[held_stack_count - 1];
}

static inline struct held_lock *held_stack_alloc(void)
{
	if (held_stack_count == JEFFPC_LOCK_STACK_DEPTH)
		return NULL;

	return &held_stack[held_stack_count++];
}

static inline void held_stack_remove(struct held_lock *held)
{
	struct held_lock *last = last_acquired_lock();

	VERIFY3P(last, !=, NULL);

	if (held != last)
		memmove(held, held + 1,
			(last - held) * sizeof(struct held_lock));

	held_stack_count--;
}

#define LOCK_DEP_GRAPH()	VERIFY0(pthread_mutex_lock(&lockdep_lock))
#define UNLOCK_DEP_GRAPH()	VERIFY0(pthread_mutex_unlock(&lockdep_lock))

#endif

/*
 * error printing
 */
static void print_invalid_call(const char *fxn, const struct lock_context *where)
{
	panic("lockdep: invalid call to %s at %s:%d", fxn, where->file,
	      where->line);
}

#define GENERATE_LOCK_MASK_ARGS(l)						\
	((l)->magic != (uintptr_t) (l)) ? 'M' : '.'

static void print_lock(struct lock *lock, const struct lock_context *where)
{
	cmn_err(CE_CRIT, "lockdep:     %s (%p) <%c> at %s:%d",
#ifdef JEFFPC_LOCK_TRACKING
		lock->name,
#else
		"<unknown>",
#endif
		lock,
		GENERATE_LOCK_MASK_ARGS(lock),
		where->file, where->line);
}

#ifdef JEFFPC_LOCK_TRACKING
static void print_lock_class(struct lock_class *lc)
{
	cmn_err(CE_CRIT, "lockdep:     class %s (%p): %zu deps", lc->name,
		lc, lc->ndeps);
}

static void print_held_locks(struct held_lock *highlight)
{
	struct held_lock *cur;
	size_t i;

	if (!held_stack_count) {
		cmn_err(CE_CRIT, "lockdep:     (no locks held)");
		return;
	}

	for_each_held_lock(i, cur) {
		struct lock *lock = cur->lock;

		cmn_err(CE_CRIT, "lockdep:  %s #%zd: %s (%p) <%c> acquired at %s:%d",
			(cur == highlight) ? "->" : "  ",
			i, lock->name, lock,
			GENERATE_LOCK_MASK_ARGS(lock),
			cur->where.file, cur->where.line);
	}
}

static void error_destroy(struct held_lock *held,
			  const struct lock_context *where)
{
	cmn_err(CE_CRIT, "lockdep: thread is trying to destroy a lock it is "
		"still holding:");
	print_lock(held->lock, where);
	cmn_err(CE_CRIT, "lockdep: while holding:");
	print_held_locks(held);

	atomic_set(&lockdep_on, 0);
}

static void error_lock(struct held_lock *held, struct lock *new,
		       const struct lock_context *where)
{
	const bool deadlock = (new == held->lock);

	if (deadlock)
		cmn_err(CE_CRIT, "lockdep: deadlock detected");
	else
		cmn_err(CE_CRIT, "lockdep: possible recursive locking detected");

	cmn_err(CE_CRIT, "lockdep: thread is trying to acquire lock:");
	print_lock(new, where);

	if (deadlock)
		cmn_err(CE_CRIT, "lockdep: but the thread is already "
			"holding it:");
	else
		cmn_err(CE_CRIT, "lockdep: but the thread is already "
			"holding a lock of same class:");

	print_held_locks(held);

	if (deadlock)
		panic("lockdep: Aborting - deadlock");

	atomic_set(&lockdep_on, 0);
}

static void error_lock_circular(struct lock *new,
				const struct lock_context *where)
{
	struct held_lock *last = last_acquired_lock();

	cmn_err(CE_CRIT, "lockdep: circular dependency detected");
	cmn_err(CE_CRIT, "lockdep: thread is trying to acquire lock of "
		"class %s (%p):", new->lc->name, new->lc);
	print_lock(new, where);
	cmn_err(CE_CRIT, "lockdep: but the thread is already holding of "
		"class %s (%p):", last->lock->lc->name, last->lock->lc);
	print_lock(last->lock, &last->where);
	cmn_err(CE_CRIT, "lockdep: which already depends on the new lock's "
		"class.");
	cmn_err(CE_CRIT, "lockdep: the reverse dependency chain:");

	atomic_set(&lockdep_on, 0);
}

static void error_unlock(struct lock *lock, const struct lock_context *where)
{
	cmn_err(CE_CRIT, "lockdep: thread is trying to release lock it "
		"doesn't hold:");
	print_lock(lock, where);
	cmn_err(CE_CRIT, "lockdep: while holding:");
	print_held_locks(NULL);
	panic("lockdep: Aborting - releasing unheld lock");
}

static void error_alloc(struct lock *lock, const struct lock_context *where,
			const char *msg)
{
	cmn_err(CE_CRIT, "lockdep: %s", msg);
	cmn_err(CE_CRIT, "lockdep: thread trying to acquire lock:");
	print_lock(lock, where);
	cmn_err(CE_CRIT, "lockdep: while holding:");
	print_held_locks(NULL);

	atomic_set(&lockdep_on, 0);
}
#endif

/*
 * dependency tracking
 */
#ifdef JEFFPC_LOCK_TRACKING
/*
 * Returns a negative int on error, 0 if there was no change to the graph,
 * and a positive int if a new dependency was added.
 */
static int add_dependency(struct lock_class *from,
			  struct lock_class *to)
{
	size_t i;

	VERIFY3P(from, !=, to);

	/* check again with the lock held */
	if (!atomic_read(&lockdep_on))
		return 0; /* pretend everything went well */

	for (i = 0; i < from->ndeps; i++)
		if (from->deps[i] == to)
			return 0; /* already present */

	if (from->ndeps >= (JEFFPC_LOCK_DEP_COUNT - 1))
		return -1;

	from->deps[from->ndeps] = to;
	from->ndeps++;

	return 1;
}

static bool __find_path(struct lock *lock,
			const struct lock_context *where,
			struct lock_class *from,
			struct lock_class *to)
{
	size_t i;

	if (from == to) {
		error_lock_circular(lock, where);
		print_lock_class(from);
		return true;
	}

	for (i = 0; i < from->ndeps; i++) {
		if (__find_path(lock, where, from->deps[i], to)) {
			print_lock_class(from);
			return true;
		}
	}

	return false;
}

static void find_path(struct lock *lock,
		      const struct lock_context *where,
		      struct lock_class *from,
		      struct lock_class *to,
		      struct held_lock *held)
{
	if (__find_path(lock, where, from, to)) {
		cmn_err(CE_CRIT, "lockdep: currently held locks:");
		print_held_locks(held);
	}
}

static bool check_circular_deps(struct lock *lock,
				const struct lock_context *where)
{
	struct held_lock *last = last_acquired_lock();
	int ret;

	if (!last)
		return false; /* no currently held locks == no deps to check */

	LOCK_DEP_GRAPH();

	ret = add_dependency(lock->lc, last->lock->lc);
	if (ret < 0)
		error_alloc(lock, where, "lock dependency count limit reached");
	else if (ret > 0)
		find_path(lock, where, last->lock->lc, lock->lc, last);

	UNLOCK_DEP_GRAPH();

	return !atomic_read(&lockdep_on);
}
#endif

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
#ifdef JEFFPC_LOCK_TRACKING
	cmn_err(CE_CRIT, "lockdep: while holding:");
	print_held_locks(NULL);
#endif
	panic("lockdep: Aborting - bad lock magic");
}

static void verify_lock_init(const struct lock_context *where, struct lock *l,
			     struct lock_class *lc)
{
	if (!l || !lc)
		print_invalid_call("MXINIT", where);

	l->magic = (uintptr_t) l;

#ifdef JEFFPC_LOCK_TRACKING
	l->lc = lc;
	l->name = where->lockname;
#endif
}

static void verify_lock_destroy(const struct lock_context *where, struct lock *l)
{
	if (!l)
		print_invalid_call("MXDESTROY", where);

	check_lock_magic(l, "destroy", where);

#ifdef JEFFPC_LOCK_TRACKING
	struct held_lock *held;
	size_t i;

	/* check that we're not holding it */
	for_each_held_lock(i, held) {
		if (held->lock == l) {
			error_destroy(held, where);
			return;
		}
	}
#endif
}

static void verify_lock_lock(const struct lock_context *where, struct lock *l)
{
	if (!l)
		print_invalid_call("MXLOCK", where);

	check_lock_magic(l, "acquire", where);

#ifdef JEFFPC_LOCK_TRACKING
	struct held_lock *held;
	size_t i;

	if (!atomic_read(&lockdep_on))
		return;

	/* check for deadlocks & recursive locking */
	for_each_held_lock(i, held) {
		if ((held->lock == l) || (held->lock->lc == l->lc)) {
			error_lock(held, l, where);
			return;
		}
	}

	/* check for circular dependencies */
	if (check_circular_deps(l, where))
		return;

	held = held_stack_alloc();
	if (!held) {
		error_alloc(l, where, "lock nesting limit reached");
		return;
	}

	held->lock = l;
	held->where = *where;
#endif
}

static void verify_lock_unlock(const struct lock_context *where, struct lock *l)
{
	if (!l)
		print_invalid_call("MXUNLOCK", where);

	check_lock_magic(l, "release", where);

#ifdef JEFFPC_LOCK_TRACKING
	struct held_lock *held;
	size_t i;

	if (!atomic_read(&lockdep_on))
		return;

	for_each_held_lock(i, held) {
		if (held->lock != l)
			continue;

		held_stack_remove(held);

		goto out;
	}

	error_unlock(l, where);
	return;

out:
	return;
#endif
}

/*
 * synch API
 */
void mxinit(const struct lock_context *where, struct lock *l,
	    struct lock_class *lc)
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
