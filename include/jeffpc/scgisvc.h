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

#ifndef __JEFFPC_SCGISVC_H
#define __JEFFPC_SCGISVC_H

#include <jeffpc/int.h>
#include <jeffpc/nvl.h>
#include <jeffpc/socksvc.h>

struct scgi {
	uint32_t id;

	int fd;

	/* request */
	struct {
		struct nvlist *headers;
		struct nvlist *query;
		size_t content_length;
		const void *body;
	} request;

	/* response */
	struct {
		enum scgi_status {
			SCGI_STATUS_OK =	200,
			SCGI_STATUS_REDIRECT =	301,
			SCGI_STATUS_NOTFOUND =	404,
		} status;
		struct nvlist *headers;
		size_t bodylen;
		void *body;
	} response;

	/* timing information */
	struct socksvc_stats conn_stats;
	struct {
		uint64_t read_header_time;	/* headers read */
		uint64_t read_body_time;	/* body read/callback started */
		uint64_t compute_time;		/* callback finished */
		uint64_t write_header_time;	/* headers written */
		uint64_t write_body_time;	/* body written */
	} scgi_stats;
};

extern int scgisvc(const char *host, uint16_t port, int nthreads,
		   void (*func)(struct scgi *, void *), void *private);

#endif
