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

#ifndef __JEFFPC_SCGI_H
#define __JEFFPC_SCGI_H

#define SCGI_CONTENT_LENGTH	"CONTENT_LENGTH"
#define SCGI_CONTENT_TYPE	"CONTENT_TYPE"
#define SCGI_DOCUMENT_URI	"DOCUMENT_URI"		/* excludes QS */
#define SCGI_HTTP_REFERER	"HTTP_REFERER"
#define SCGI_HTTP_USER_AGENT	"HTTP_USER_AGENT"
#define SCGI_QUERY_STRING	"QUERY_STRING"
#define SCGI_REMOTE_ADDR	"REMOTE_ADDR"
#define SCGI_REMOTE_PORT	"REMOTE_PORT"
#define SCGI_REQUEST_METHOD	"REQUEST_METHOD"
#define SCGI_REQUEST_URI	"REQUEST_URI"		/* includes QS */
#define SCGI_SERVER_NAME	"SERVER_NAME"
#define SCGI_SERVER_PORT	"SERVER_PORT"
#define SCGI_SERVER_PROTOCOL	"SERVER_PROTOCOL"	/* e.g., HTTP/1.1 */

#endif
