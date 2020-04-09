/*
 * Copyright (c) 2017-2020 Josef 'Jeff' Sipek <jeffpc@josefsipek.net>
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

#include <unistd.h>

#include <jeffpc/error.h>
#include <jeffpc/taskq.h>
#include <jeffpc/cstr.h>
#include <jeffpc/mem.h>

static LOCK_CLASS(taskq_lc);

static struct mem_cache *taskq_cache;

static void __attribute__((constructor)) init_taskq_subsys(void)
{
	taskq_cache = mem_cache_create("taskq-cache", sizeof(struct taskq), 0);
	ASSERT(!IS_ERR(taskq_cache));
}

static void enqueue(struct taskq *tq, struct taskq_item *item)
{
	list_insert_tail(&tq->queue, item);
	tq->queue_len++;
}

static struct taskq_item *dequeue(struct taskq *tq)
{
	struct taskq_item *item;

	item = list_remove_head(&tq->queue);
	if (!item)
		return NULL;

	tq->queue_len--;

	return item;
}

static void *taskq_worker(void *arg)
{
	struct taskq *tq = arg;

	MXLOCK(&tq->lock);

	/* process */
	while (!tq->shutdown) {
		struct taskq_item *item;

		item = dequeue(tq);
		if (!item) {
			CONDWAIT(&tq->cond_parent2worker, &tq->lock);
			continue;
		}

		CONDBCAST(&tq->cond_worker2parent);
		MXUNLOCK(&tq->lock);

		item->fxn(item->arg);

		free(item);

		MXLOCK(&tq->lock);

		tq->processed++;
	}

	MXUNLOCK(&tq->lock);

	return NULL;
}

static int start_threads(struct taskq *tq)
{
	int ret = 0;
	long i;

	VERIFY(tq->nthreads);

	MXLOCK(&tq->lock);

	for (i = 0; i < tq->nthreads; i++) {
		ret = xthr_create(&tq->threads[i], taskq_worker, tq);
		if (ret)
			break;

		tq->nstarted_threads++;
	}

	MXUNLOCK(&tq->lock);

	return ret;
}

static long get_nthreads(long nthreads)
{
	if (nthreads == -1) {
		nthreads = sysconf(_SC_NPROCESSORS_ONLN);
		if (nthreads == -1)
			return -errno;
	}

	if (nthreads <= 0)
		return -EINVAL;

	return nthreads;
}

/*
 * Create a taskq with a given number of threads.  If the number of threads
 * is -1, the current number of online processors is used instead.  All
 * threads are created immediately.
 */
struct taskq *taskq_create_fixed(const char *name, long nthreads)
{
	struct taskq *tq;
	int ret;

	nthreads = get_nthreads(nthreads);
	if (nthreads < 0)
		return ERR_PTR(nthreads);

	VERIFY(nthreads);

	tq = mem_cache_alloc(taskq_cache);
	if (!tq)
		goto err;

	tq->threads = calloc(nthreads, sizeof(pthread_t));
	if (!tq->threads)
		goto err_free;

	strcpy_safe(tq->name, name, sizeof(tq->name));
	tq->nthreads = nthreads;
	tq->nstarted_threads = 0;
	tq->queue_len = 0;
	tq->shutdown = false;
	tq->processed = 0;

	list_create(&tq->queue, sizeof(struct taskq_item),
		    offsetof(struct taskq_item, node));
	MXINIT(&tq->lock, &taskq_lc);
	CONDINIT(&tq->cond_worker2parent);
	CONDINIT(&tq->cond_parent2worker);

	ret = start_threads(tq);
	if (ret) {
		taskq_destroy(tq);
		return ERR_PTR(ret);
	}

	return tq;

err_free:
	mem_cache_free(taskq_cache, tq);
err:
	return ERR_PTR(-ENOMEM);
}

/*
 * Allocate and enqueue a taskq item.  Once a worker thread is available, it
 * will call fxn(arg) and free the item.
 */
int taskq_dispatch(struct taskq *tq, void (*fxn)(void *), void *arg)
{
	struct taskq_item *item;

	item = malloc(sizeof(struct taskq_item));
	if (!item)
		return -ENOMEM;

	item->fxn = fxn;
	item->arg = arg;

	MXLOCK(&tq->lock);
	enqueue(tq, item);
	CONDSIG(&tq->cond_parent2worker);
	MXUNLOCK(&tq->lock);

	return 0;
}

/*
 * Wait for the queue of work items to drain.
 */
void taskq_wait(struct taskq *tq)
{
	MXLOCK(&tq->lock);

	while (!list_is_empty(&tq->queue))
		CONDWAIT(&tq->cond_worker2parent, &tq->lock);

	VERIFY(list_is_empty(&tq->queue));

	MXUNLOCK(&tq->lock);
}

/*
 * Destroy the taskq.  The queue must be empty at this point.
 */
void taskq_destroy(struct taskq *tq)
{
	long i;

	MXLOCK(&tq->lock);

	/* the queue should be empty */
	VERIFY(list_is_empty(&tq->queue));

	/* make everyone aware that we are shutting down */
	tq->shutdown = true;
	CONDBCAST(&tq->cond_parent2worker);

	MXUNLOCK(&tq->lock);

	/* wait for all the threads to terminate */
	for (i = 0; i < tq->nstarted_threads; i++)
		xthr_join(tq->threads[i], NULL);

	/* free */
	CONDDESTROY(&tq->cond_parent2worker);
	CONDDESTROY(&tq->cond_worker2parent);
	MXDESTROY(&tq->lock);
	list_destroy(&tq->queue);
	free(tq->threads);
	mem_cache_free(taskq_cache, tq);
}
