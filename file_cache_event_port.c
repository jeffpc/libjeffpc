/*
 * Copyright (c) 2014-2019 Josef 'Jeff' Sipek <jeffpc@josefsipek.net>
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

#include <fcntl.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <stddef.h>
#include <stdbool.h>
#include <port.h>

#include <jeffpc/atomic.h>
#include <jeffpc/val.h>
#include <jeffpc/synch.h>
#include <jeffpc/thread.h>
#include <jeffpc/refcnt.h>
#include <jeffpc/io.h>
#include <jeffpc/list.h>
#include <jeffpc/mem.h>
#include <jeffpc/rbtree.h>
#include <jeffpc/file-cache.h>

static LOCK_CLASS(file_lock_lc);
static LOCK_CLASS(file_node_lc);

static struct rb_tree file_cache;
static struct lock file_lock;

static atomic64_t current_revision;

static int filemon_port;

static struct mem_cache *file_node_cache;

struct file_node {
	char *name;			/* the filename */
	struct rb_node node;
	refcnt_t refcnt;
	struct lock lock;

	/* everything else is protected by the lock */
	struct str *contents;		/* cache file contents if allowed */
	struct stat stat;		/* the stat info of the cached file */
	bool needs_reload;		/* caching stale data */
	uint64_t cache_rev;		/* cache version */
	struct file_obj fobj;		/* FEN port object */
};

static void fn_free(struct file_node *node);
static int __reload(struct file_node *node);

REFCNT_INLINE_FXNS(struct file_node, fn, refcnt, fn_free, NULL)

static void print_event(const char *fname, int event)
{
	cmn_err(CE_DEBUG, "%s: %s", __func__, fname);
	if (event & FILE_ACCESS)
		cmn_err(CE_DEBUG, "\tFILE_ACCESS");
	if (event & FILE_MODIFIED)
		cmn_err(CE_DEBUG, "\tFILE_MODIFIED");
	if (event & FILE_ATTRIB)
		cmn_err(CE_DEBUG, "\tFILE_ATTRIB");
	if (event & FILE_DELETE)
		cmn_err(CE_DEBUG, "\tFILE_DELETE");
	if (event & FILE_RENAME_TO)
		cmn_err(CE_DEBUG, "\tFILE_RENAME_TO");
	if (event & FILE_RENAME_FROM)
		cmn_err(CE_DEBUG, "\tFILE_RENAME_FROM");
	if (event & UNMOUNTED)
		cmn_err(CE_DEBUG, "\tUNMOUNTED");
	if (event & MOUNTEDOVER)
		cmn_err(CE_DEBUG, "\tMOUNTEDOVER");
}

static void process_file(struct file_node *node, int events)
{
	struct file_obj *fobj = &node->fobj;
	struct stat statbuf;

	MXLOCK(&node->lock);

	if (!(events & FILE_EXCEPTION)) {
		int ret;

		ret = xstat(fobj->fo_name, &statbuf);
		if (ret) {
			cmn_err(CE_DEBUG, "failed to stat '%s' error: %s\n",
				fobj->fo_name, xstrerror(ret));
			goto free;
		}
	}

	if (events) {
		/* something changed, we need to reload */
		node->needs_reload = true;

		print_event(fobj->fo_name, events);

		/*
		 * If the file went away, we could rely on reloading to deal
		 * with it.  Or we can just remove the node and have the
		 * next caller read it from disk.  We take the later
		 * approach.
		 */
		if (events & FILE_EXCEPTION)
			goto free;

		/*
		 * Because the cached data is invalid (and therefore
		 * useless), we can free it now and avoid having it linger
		 * around, ending up core files, etc.
		 */
		str_putref(node->contents);
		node->contents = NULL;
	}

	/* re-register */
	fobj->fo_atime = statbuf.st_atim;
	fobj->fo_mtime = statbuf.st_mtim;
	fobj->fo_ctime = statbuf.st_ctim;

	if (port_associate(filemon_port, PORT_SOURCE_FILE, (uintptr_t) fobj,
			   (FILE_MODIFIED | FILE_ATTRIB), node) == -1) {
		cmn_err(CE_DEBUG, "failed to register file '%s' errno %d",
			fobj->fo_name, errno);
		goto free;
	}

	MXUNLOCK(&node->lock);

	return;

free:
	/*
	 * If there was an error, remove the node from the cache.  The next
	 * time someone tries to get this file, they'll pull it in from
	 * disk.
	 */
	MXLOCK(&file_lock);
	rb_remove(&file_cache, node);
	MXUNLOCK(&file_lock);

	MXUNLOCK(&node->lock);
	fn_putref(node); /* put the cache's reference */
}

static void *filemon(void *arg)
{
	port_event_t pe;

	while (!port_get(filemon_port, &pe, NULL)) {
		switch (pe.portev_source) {
			case PORT_SOURCE_FILE:
				process_file(pe.portev_user,
					     pe.portev_events);
				break;
			default:
				panic("unexpected event source");
		}
	}

	return NULL;
}

static int filename_cmp(const void *va, const void *vb)
{
	const struct file_node *a = va;
	const struct file_node *b = vb;
	int ret;

	ret = strcmp(a->name, b->name);
	if (ret < 0)
		return -1;
	if (ret > 0)
		return 1;
	return 0;
}

int file_cache_init(void)
{
	int ret;

	MXINIT(&file_lock, &file_lock_lc);

	rb_create(&file_cache, filename_cmp, sizeof(struct file_node),
		  offsetof(struct file_node, node));

	file_node_cache = mem_cache_create("file-node-cache",
					   sizeof(struct file_node), 0);
	if (IS_ERR(file_node_cache)) {
		ret = PTR_ERR(file_node_cache);
		goto err;
	}

	/* start the file event monitor */
	filemon_port = port_create();
	if (filemon_port == -1) {
		ret = -errno;
		goto err_cache;
	}

	ret = xthr_create(NULL, filemon, NULL);
	if (ret)
		goto err_port;

	return 0;

err_port:
	xclose(filemon_port);

err_cache:
	mem_cache_destroy(file_node_cache);

err:
	rb_destroy(&file_cache);

	return ret;
}

static struct file_node *fn_alloc(const char *name)
{
	struct file_node *node;

	node = mem_cache_alloc(file_node_cache);
	if (!node)
		return NULL;

	node->name = strdup(name);
	if (!node->name)
		goto err;

	node->fobj.fo_name = node->name;
	node->contents = NULL;
	node->needs_reload = true;
	node->cache_rev = atomic_inc(&current_revision);

	MXINIT(&node->lock, &file_node_lc);
	refcnt_init(&node->refcnt, 1);

	return node;

err:
	mem_cache_free(file_node_cache, node);
	return NULL;
}

static void fn_free(struct file_node *node)
{
	if (!node)
		return;

	str_putref(node->contents);
	free(node->name);
	mem_cache_free(file_node_cache, node);
}

static int __reload(struct file_node *node)
{
	char *tmp;

	if (!node->needs_reload)
		return 0;

	/* free the previous */
	str_putref(node->contents);
	node->contents = NULL;

	/* read the current */
	tmp = read_file_common(AT_FDCWD, node->name, &node->stat);
	if (IS_ERR(tmp)) {
		cmn_err(CE_DEBUG, "file (%s) read error: %s", node->name,
			xstrerror(PTR_ERR(tmp)));
		return PTR_ERR(tmp);
	}

	node->contents = str_alloc(tmp);
	if (IS_ERR(node->contents)) {
		int ret = PTR_ERR(node->contents);

		cmn_err(CE_DEBUG, "file (%s) str_alloc error: %s",
			node->name, xstrerror(ret));
		free(tmp);

		node->contents = NULL;
		return ret;
	}

	node->needs_reload = false;
	node->cache_rev = atomic_inc(&current_revision);

	return 0;
}

static struct file_node *load_file(const char *name)
{
	struct file_node *node;
	int ret;

	node = fn_alloc(name);
	if (!node)
		return ERR_PTR(-ENOMEM);

	if ((ret = __reload(node))) {
		fn_free(node);
		return ERR_PTR(ret);
	}

	return node;
}

struct str *file_cache_get(const char *name, uint64_t *rev)
{
	struct file_node *out, *tmp;
	struct file_node key;
	struct str *str;
	struct rb_cookie where;
	int ret;

	key.name = (char *) name;

	/* do we have it? */
	MXLOCK(&file_lock);
	out = rb_find(&file_cache, &key, NULL);
	fn_getref(out);
	MXUNLOCK(&file_lock);

	/* already had it, so return that */
	if (out)
		goto output;

	/* have to load it from disk...*/
	out = load_file(name);
	if (IS_ERR(out))
		return ERR_CAST(out);

	MXLOCK(&out->lock);

	/* ...and insert it into the cache */
	MXLOCK(&file_lock);
	tmp = rb_find(&file_cache, &key, &where);
	if (tmp) {
		/*
		 * uh oh, someone beat us to it; free our copy & return
		 * existing
		 */
		fn_getref(tmp);
		MXUNLOCK(&file_lock);

		/* release the node we allocated */
		MXUNLOCK(&out->lock);
		fn_putref(out);

		/* work with the node found in the tree */
		out = tmp;
		goto output;
	}

	/* get a ref for the cache */
	fn_getref(out);

	rb_insert_here(&file_cache, out, &where);

	MXUNLOCK(&file_lock);
	MXUNLOCK(&out->lock);

	/*
	 * Ok!  The node is in the cache.
	 */

	/* start watching */
	process_file(out, 0);

output:
	/* get a reference for the string */
	MXLOCK(&out->lock);
	ret = __reload(out);
	if (ret) {
		/* there was an error reloading - bail */
		MXUNLOCK(&out->lock);
		fn_putref(out);
		return ERR_PTR(ret);
	}

	str = str_getref(out->contents);

	/* inform the caller about which version we're returning */
	if (rev)
		*rev = out->cache_rev;
	MXUNLOCK(&out->lock);

	/* put the reference for the file node */
	fn_putref(out);

	return str;
}

bool file_cache_has_newer(const char *name, uint64_t rev)
{
	struct file_node *out;
	struct file_node key;
	bool ret;

	key.name = (char *) name;

	/* do we have it? */
	MXLOCK(&file_lock);
	out = rb_find(&file_cache, &key, NULL);
	fn_getref(out);
	MXUNLOCK(&file_lock);

	/*
	 * We don't have it cached (which is weird/a bug), so let's pretend
	 * that there is a newer version...just in case.
	 */
	if (!out)
		return true;

	MXLOCK(&out->lock);
	ret = out->needs_reload || (rev != out->cache_rev);
	MXUNLOCK(&out->lock);

	/* put the reference for the file node */
	fn_putref(out);

	return ret;
}

void uncache_all_files(void)
{
	struct file_node *cur;
	struct rb_cookie cookie;

	MXLOCK(&file_lock);
	memset(&cookie, 0, sizeof(cookie));
	while ((cur = rb_destroy_nodes(&file_cache, &cookie)))
		fn_putref(cur);
	MXUNLOCK(&file_lock);
}
