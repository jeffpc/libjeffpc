/*
 * Copyright (c) 2015-2016 Josef 'Jeff' Sipek <jeffpc@josefsipek.net>
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

#ifndef __JEFFPC_SEXPR_H
#define __JEFFPC_SEXPR_H

#include <stdbool.h>

#include <jeffpc/val.h>
#include <jeffpc/str.h>

extern struct val *parse_sexpr(const char *str, size_t len);
extern struct str *sexpr_dump(struct val *lv, bool raw);
extern void sexpr_dump_file(FILE *out, struct val *lv, bool raw);
extern struct val *sexpr_array_to_list(struct val **vals, int nvals);

static inline struct val *parse_sexpr_str(struct str *str)
{
	if (!str)
		return NULL;

	return parse_sexpr(str_cstr(str), str_len(str));
}

static inline struct val *parse_sexpr_cstr(const char *str)
{
	if (!str)
		return NULL;

	return parse_sexpr(str, strlen(str));
}

extern struct val *sexpr_car(struct val *val);
extern struct val *sexpr_cdr(struct val *val);
extern struct val *sexpr_assoc(struct val *val, const char *name);
extern bool sexpr_equal(struct val *lhs, struct val *rhs);

/*
 * Assorted helpers to make alists more pleasant to use.
 */
extern struct val *sexpr_alist_lookup_val(struct val *lv, const char *name);
extern struct str *sexpr_alist_lookup_str(struct val *lv, const char *name);
extern uint64_t sexpr_alist_lookup_int(struct val *lv, const char *name,
				       bool *found);
extern struct val *sexpr_alist_lookup_list(struct val *lv, const char *name);

#endif
