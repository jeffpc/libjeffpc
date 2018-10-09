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

#ifndef __JEFFPC_UUID_H
#define __JEFFPC_UUID_H

#include <inttypes.h>
#include <stdbool.h>
#include <rpc/rpc.h>

/* length of a string-ified UUID including the nul-terminator */
#define XUUID_PRINTABLE_STRING_LENGTH	37

struct xuuid {
	uint8_t raw[16];
};

extern const struct xuuid xuuid_null_uuid;

extern void xuuid_clear(struct xuuid *uuid);
extern int xuuid_compare(const struct xuuid *u1, const struct xuuid *u2);
extern void xuuid_generate(struct xuuid *uuid);

/* returns true if uuid was successfully parsed */
extern bool xuuid_parse(struct xuuid *u, const char *in);
extern void xuuid_unparse(const struct xuuid *u, char *out);

extern bool_t xdr_xuuid(XDR *xdr, struct xuuid *obj);

#endif
