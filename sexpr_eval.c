/*
 * Copyright (c) 2016-2017 Josef 'Jeff' Sipek <jeffpc@josefsipek.net>
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

#include <jeffpc/sexpr.h>

struct builtin_fxn {
	const char *name;
	struct val *(*f)(struct val *args, struct sexpr_eval_env *env);

	/* expected number of arguments; -1 indicates any length is ok */
	ssize_t arglen;
};

#define __REDUCE(fname, alloc, t, valmember, ident, op)				\
static struct val *fname(struct val *args, struct sexpr_eval_env *env)		\
{										\
	struct val *ret;							\
										\
	ret = alloc(ident);							\
										\
	while (args) {								\
		struct val *el = sexpr_car(val_getref(args));			\
		struct val *next = sexpr_cdr(args);				\
										\
		while (el && (el->type == VT_CONS || el->type == VT_SYM))	\
			el = sexpr_eval(el, env);				\
										\
		ASSERT(el);							\
		ASSERT3U(el->type, ==, t);					\
										\
		ret->valmember = ret->valmember op el->valmember;		\
										\
		val_putref(el);							\
		args = next;							\
	}									\
										\
	return ret;								\
}

#define BOOL_REDUCE(fname, ident, op) \
	__REDUCE(fname, VAL_ALLOC_BOOL, VT_BOOL, b, ident, op)

BOOL_REDUCE(fxn_or,  false, ||)
BOOL_REDUCE(fxn_and, true,  &&)

#define INT_REDUCE(fname, ident, op) \
	__REDUCE(fname, VAL_ALLOC_INT, VT_INT, i, ident, op)

INT_REDUCE(fxn_add,  0, +)
INT_REDUCE(fxn_mult, 1, *)

static struct val *fxn_quote(struct val *args, struct sexpr_eval_env *env)
{
	return sexpr_car(args);
}

static struct val *fxn_cxr(struct val *args, struct sexpr_eval_env *env,
			   struct val *(*cxr)(struct val *))
{
	/*
	 * The args argument contains the cdr of the whole expression.  For
	 * example, if we tried to evaluate:
	 *
	 *   (car '(a b))
	 *
	 * aka.
	 *
	 *   (car (quote (a b)))
	 *
	 * args will contain:
	 *
	 *   ((quote (a b)))
	 *
	 * That is why we do a car on it first before calling eval.
	 *
	 * Note: This makes sense because in general functions can take any
	 * number of arguments - it's just that car and cdr take only one.
	 * So, we need to take the first element of the list of arguments -
	 * and we get that by calling car.
	 */
	return cxr(sexpr_eval(sexpr_car(args), env));
}

static struct val *fxn_car(struct val *args, struct sexpr_eval_env *env)
{
	return fxn_cxr(args, env, sexpr_car);
}

static struct val *fxn_cdr(struct val *args, struct sexpr_eval_env *env)
{
	return fxn_cxr(args, env, sexpr_cdr);
}

static struct val *fxn_equal(struct val *args, struct sexpr_eval_env *env)
{
	struct val *a, *b;

	a = sexpr_eval(sexpr_nth(val_getref(args), 1), env);
	b = sexpr_eval(sexpr_nth(args, 2), env);

	return VAL_ALLOC_BOOL(sexpr_equal(a, b));
}

static struct val *fxn_if(struct val *args, struct sexpr_eval_env *env)
{
	struct val *c;
	struct val *t, *f;
	struct val *ret;

	c = sexpr_nth(val_getref(args), 1);
	t = sexpr_nth(val_getref(args), 2);
	f = sexpr_nth(args, 3);

	c = sexpr_eval(c, env);

	VERIFY3U(c->type, ==, VT_BOOL);

	ret = c->b ? val_getref(t) : val_getref(f);

	val_putref(c);
	val_putref(t);
	val_putref(f);

	return sexpr_eval(ret, env);
}

static struct builtin_fxn builtins[] = {
	{ "and",   fxn_and,   -1, },
	{ "or",    fxn_or,    -1, },
	{ "&&",    fxn_and,   -1, },
	{ "||",    fxn_or,    -1, },
	{ "+",     fxn_add,   -1, },
	{ "*",     fxn_mult,  -1, },
	{ "quote", fxn_quote,  1, },
	{ "car",   fxn_car,    1, },
	{ "cdr",   fxn_cdr,    1, },
	{ "=",     fxn_equal,  2, },
	{ "==",    fxn_equal,  2, },
	{ "if",    fxn_if,     3, },
	{ NULL, },
};

static struct builtin_fxn *fxnlookup_builtin(struct str *name)
{
	size_t i;

	for (i = 0; builtins[i].name; i++)
		if (!strcmp(builtins[i].name, str_cstr(name)))
			return &builtins[i];

	return NULL;
}

static struct val *eval_cons(struct val *expr, struct sexpr_eval_env *env)
{
	struct builtin_fxn *fxn;
	struct str *name;
	struct val *args;
	struct val *op;

	op = sexpr_car(val_getref(expr));
	args = sexpr_cdr(expr);

	ASSERT(op);
	switch (op->type) {
		case VT_INT:
			panic("function name cannot be a VT_INT (%"PRIu64")",
			      op->i);
		case VT_CHAR:
			panic("function name cannot be a VT_CHAR (%"PRIu64")",
			      op->i);
		case VT_STR:
			panic("function name cannot be a VT_STR (\"%s\")",
			      str_cstr(op->str));
		case VT_BOOL:
			panic("function name cannot be a VT_BOOL (%s)",
			      op->b ? "true" : "false");
		case VT_CONS:
			panic("function name cannot be a VT_CONS");
		case VT_SYM:
			break; /* ok */
	}

	name = str_getref(op->str);
	val_putref(op);

	if (env->fxnlookup) {
		fxn = env->fxnlookup(name, env);
		if (fxn)
			goto found;
	}

	fxn = fxnlookup_builtin(name);
	if (fxn)
		goto found;

	panic("unknown function '%s'", str_cstr(name));

found:
	str_putref(name);

	if (fxn->arglen != -1) {
		size_t got;

		got = sexpr_length(val_getref(args));

		if (got != fxn->arglen)
			panic("'%s' not given the right number of arguments "
			      "(expected %d, got %d)", fxn->name, fxn->arglen,
			      got);
	}

	return fxn->f(args, env);
}

struct val *sexpr_eval(struct val *expr,
		       struct sexpr_eval_env *env)
{
	static struct sexpr_eval_env emptyenv;

	if (!expr)
		return NULL;

	if (!env)
		env = &emptyenv;

	switch (expr->type) {
		case VT_INT:
		case VT_STR:
		case VT_BOOL:
		case VT_CHAR:
			return expr;
		case VT_SYM: {
			struct str *name;

			if (!env->symlookup)
				panic("VT_SYM requires non-NULL symlookup "
				      "function in the environment");

			name = str_getref(expr->str);
			val_putref(expr);

			/*
			 * Symbol lookup returns a value (not an expression)
			 * therefore we don't want to evaluate it.
			 */
			return env->symlookup(name, env);
		}
		case VT_CONS:
			return eval_cons(expr, env);
	}

	panic("impossible!");
}
