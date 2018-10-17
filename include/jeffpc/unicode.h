/*
 * Copyright (c) 2018 Josef 'Jeff' Sipek <jeffpc@josefsipek.net>
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

#ifndef __JEFFPC_UTF8_H
#define __JEFFPC_UTF8_H

#include <stdint.h>
#include <stdlib.h>
#include <stdbool.h>
#include <unistd.h>

extern size_t utf8_to_utf32(const char *in, size_t inlen, uint32_t *out);
extern ssize_t utf32_to_utf8(uint32_t cp, char *buf, size_t buflen);

extern int utf8_is_valid_str(const char *src, size_t slen);

static inline bool utf32_is_valid(uint32_t cp)
{
	/* UTF-16 surrogates */
	if ((cp >= 0xd800) && (cp <= 0xdfff))
		return false;

	return cp <= 0x10ffff;
}

#endif
