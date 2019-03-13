/*
 * Copyright (c) 2019 Josef 'Jeff' Sipek <jeffpc@josefsipek.net>
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

#include <jeffpc/file-cache.h>
#include <jeffpc/io.h>

/*
 * This is the fallback implementation.  It doesn't actually do any caching,
 * it simply reads the file contents every time.
 */

#pragma weak file_cache_init
int file_cache_init(void)
{
	/* nothing to do */
	return 0;
}

#pragma weak file_cache_get
struct str *file_cache_get(const char *name, uint64_t *rev)
{
	struct stat statbuf;
	char *tmp;

	if (rev)
		*rev = 0;

	tmp = read_file_common(AT_FDCWD, name, &statbuf);
	if (IS_ERR(tmp))
		return ERR_CAST(tmp);

	return str_alloc(tmp);
}

#pragma weak file_cache_has_newer
bool file_cache_has_newer(const char *name, uint64_t rev)
{
	/* pretend that the file has changed to make the caller read it again */
	return true;
}

#pragma weak file_cache_uncache_all
void file_cache_uncache_all(void)
{
	/* nothing to do */
}
