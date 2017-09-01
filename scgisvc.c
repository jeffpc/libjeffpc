/*
 * Copyright (c) 2014-2017 Josef 'Jeff' Sipek <jeffpc@josefsipek.net>
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

#include <jeffpc/error.h>
#include <jeffpc/atomic.h>
#include <jeffpc/io.h>
#include <jeffpc/mem.h>
#include <jeffpc/time.h>
#include <jeffpc/scgisvc.h>
#include <jeffpc/socksvc.h>
#include <jeffpc/scgi.h>
#include <jeffpc/qstring.h>

struct scgiargs {
	void (*func)(struct scgi *, void *);
	void *private;
};

static struct mem_cache *scgisvc_cache;
static atomic_t scgi_request_ids;

static void scgi_free(struct scgi *req);

static void __attribute__((constructor)) init_scgisvc_subsys(void)
{
	scgisvc_cache = mem_cache_create("scgisvc-cache",
					 sizeof(struct scgi), 0);
	ASSERT(!IS_ERR(scgisvc_cache));
}

/*
 * SCGI protocol parser
 */

static int read_netstring_length(struct scgi *req, size_t *len)
{
	int ret;
	size_t v;

	v = 0;

	for (;;) {
		char c;

		ret = xread(req->fd, &c, sizeof(c));
		if (ret)
			return ret;

		switch (c) {
			case ':':
				*len = v;
				return 0;
			case '0': case '1': case '2': case '3':
			case '4': case '5': case '6': case '7':
			case '8': case '9':
				v = (v * 10) + (c - '0');
				break;
			default:
				return -EINVAL;
		}
	}
}

static int read_netstring_string(struct scgi *req, size_t len)
{
	char *cur, *end;
	char *buf;
	int ret;

	/*
	 * Receive the string.
	 */

	buf = malloc(len + 1);
	if (!buf)
		return -ENOMEM;

	ret = xread(req->fd, buf, len + 1);
	if (ret)
		goto err;

	if (buf[len] != ',') {
		ret = -EINVAL;
		goto err;
	}

	buf[len] = '\0';

	/*
	 * Now, parse the received string.
	 */

	cur = buf;
	end = buf + len;

	while (cur < end) {
		char *name, *val;

		/* TODO: audit this loop for correctness / safety */

		name = cur;
		val = cur + strlen(name) + 1;

		ret = nvl_set_cstr_dup(req->request.headers, name, val);
		if (ret)
			break;

		cur = val + strlen(val) + 1;
	}

err:
	free(buf);

	return ret;
}

static int read_netstring(struct scgi *req)
{
	size_t len;
	int ret;

	ret = read_netstring_length(req, &len);
	if (ret)
		return ret;

	return read_netstring_string(req, len);
}

/*
 * Header conversion
 */

static const struct nvl_convert_info scgi_convert_headers[] = {
	{ .name = "SCGI",		.tgt_type = NVT_INT, },
	{ .name = SCGI_CONTENT_LENGTH,	.tgt_type = NVT_INT, },
	{ .name = SCGI_REMOTE_PORT,	.tgt_type = NVT_INT, },
	{ .name = SCGI_SERVER_PORT,	.tgt_type = NVT_INT, },
	{ .name = NULL, },
};

static int parse_headers(struct scgi *req)
{
	uint64_t i;
	int ret;

	ret = nvl_convert(req->request.headers, scgi_convert_headers);
	if (ret)
		return ret;

	ret = nvl_lookup_int(req->request.headers, "SCGI", &i);
	if (ret)
		return ret;
	if (i != 1)
		return -EINVAL;

	ret = nvl_lookup_int(req->request.headers, SCGI_CONTENT_LENGTH, &i);
	if (ret)
		return ret;
	if (i > SIZE_MAX)
		return -EINVAL;

	req->request.content_length = i;

	return 0;
}

static int parse_qstring(struct scgi *req)
{
	struct str *qs;
	int ret;

	qs = nvl_lookup_str(req->request.headers, SCGI_QUERY_STRING);
	if (IS_ERR(qs)) {
		ret = PTR_ERR(qs);

		return (ret == -ENOENT) ? 0 : ret;
	}

	ret = qstring_parse(req->request.query, str_cstr(qs));

	str_putref(qs);

	return ret;
}

static int scgi_read_headers(struct scgi *req)
{
	int ret;

	ret = read_netstring(req);
	if (ret)
		return ret;

	ret = parse_headers(req);
	if (ret)
		return ret;

	return parse_qstring(req);
}

static int scgi_read_body(struct scgi *req)
{
	char *buf;
	int ret;

	if (!req->request.content_length)
		return 0;

	buf = malloc(req->request.content_length + 1);
	if (!buf)
		return -ENOMEM;

	ret = xread(req->fd, buf, req->request.content_length);
	if (ret) {
		free(buf);
		return ret;
	}

	buf[req->request.content_length] = '\0';

	req->request.body = buf;

	return 0;
}

static int scgi_write_headers(struct scgi *req)
{
	const struct nvpair *header;
	char tmp[256];
	int ret;

	/* return status */
	snprintf(tmp, sizeof(tmp), "Status: %u\n", req->response.status);
	ret = xwrite_str(req->fd, tmp);
	if (ret)
		return ret;

	/* write out accumulated headers */
	nvl_for_each(header, req->response.headers) {
		struct str *str;

		str = nvpair_value_str(header);
		if (IS_ERR(str))
			return PTR_ERR(str);

		snprintf(tmp, sizeof(tmp), "%s: %s\n",
			 nvpair_name(header),
			 str_cstr(str));

		str_putref(str);

		ret = xwrite_str(req->fd, tmp);
		if (ret)
			return ret;
	}

	/* separate headers from body */
	return xwrite_str(req->fd, "\n");
}

static int scgi_write_body(struct scgi *req)
{
	return xwrite(req->fd, req->response.body, req->response.bodylen);
}

static struct scgi *scgi_alloc(int fd)
{
	struct scgi *req;

	req = mem_cache_alloc(scgisvc_cache);
	if (!req)
		return ERR_PTR(-ENOMEM);

	memset(req, 0, sizeof(struct scgi));

	req->id = atomic_inc(&scgi_request_ids);
	req->fd = fd;

	req->request.headers = nvl_alloc();
	req->request.query = nvl_alloc();
	req->request.content_length = 0;
	req->request.body = NULL;
	req->response.status = SCGI_STATUS_OK;
	req->response.headers = nvl_alloc();
	req->response.bodylen = 0;
	req->response.body = NULL;

	if (!req->request.headers || !req->request.query ||
	    !req->response.headers) {
		scgi_free(req);
		return ERR_PTR(-ENOMEM);
	}

	return req;
}

static void scgi_free(struct scgi *req)
{
	nvl_putref(req->request.headers);
	nvl_putref(req->request.query);
	nvl_putref(req->response.headers);

	/* NOTE: Do *not* close the fd, it'll be closed by socksvc */

	mem_cache_free(scgisvc_cache, req);
}

static void scgi_conn(int fd, struct socksvc_stats *sockstats, void *private)
{
	struct scgiargs *args = private;
	struct scgi *req;
	int ret;

	req = scgi_alloc(fd);
	if (IS_ERR(req)) {
		ret = PTR_ERR(req);
		goto out;
	}

	req->conn_stats = *sockstats;

	ret = scgi_read_headers(req);
	if (ret)
		goto out_free;

	req->scgi_stats.read_header_time = gettime();

	ret = scgi_read_body(req);
	if (ret)
		goto out_free;

	req->scgi_stats.read_body_time = gettime();

	args->func(req, args->private);

	req->scgi_stats.compute_time = gettime();

	ret = scgi_write_headers(req);
	if (ret)
		goto out_free;

	req->scgi_stats.write_header_time = gettime();

	ret = scgi_write_body(req);

	req->scgi_stats.write_body_time = gettime();

out_free:
	scgi_free(req);

out:
	if (ret)
		cmn_err(CE_INFO, "%s failed: %s", __func__, xstrerror(ret));
}

int scgisvc(const char *host, uint16_t port, int nthreads,
	    void (*func)(struct scgi *, void *), void *private)
{
	struct scgiargs args = {
		.func = func,
		.private = private,
	};

	return socksvc(host, port, nthreads, scgi_conn, &args);
}
