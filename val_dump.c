/*
 * Copyright (c) 2014-2018 Josef 'Jeff' Sipek <jeffpc@josefsipek.net>
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

#include <ctype.h>

#include <jeffpc/val.h>
#include <jeffpc/nvl.h>

#define INDENT_STEP	5

static inline const char *typename(int type)
{
	static __thread char badname[10];

#define TYPE(t, h)	case t: return human ? #h : #t

	switch ((enum val_type) type) {
		TYPE(VT_ARRAY, array);
		TYPE(VT_BLOB,  blob);
		TYPE(VT_BOOL,  bool);
		TYPE(VT_CHAR,  char);
		TYPE(VT_CONS,  cons);
		TYPE(VT_INT,   int);
		TYPE(VT_NULL,  null);
		TYPE(VT_NVL,   nvlist);
		TYPE(VT_STR,   string);
		TYPE(VT_SYM,   symbol);
	}

#undef TYPE

	snprintf(badname, sizeof(badname), "<%d>", type);
	return badname;
}

static inline void doindent(FILE *out, int indent)
{
	fprintf(out, "%*s", INDENT_STEP * indent, "");
}

static void do_val_dump_file(FILE *out, struct val *val, int indent)
{
	if (!val) {
		fprintf(out, "\n");
		return;
	}

	fprintf(out, "type=%s", typename(val->type));

	switch (val->type) {
		case VT_NULL:
			fprintf(out, "\n");
			break;
		case VT_STR:
		case VT_SYM:
			fprintf(out, "\n");
			doindent(out, indent);
			fprintf(out, "value='%s'\n", val_cstr(val));
			break;
		case VT_INT:
			fprintf(out, "\n");
			doindent(out, indent);
			fprintf(out, "value=%"PRIu64"\n", val->i);
			break;
		case VT_BOOL:
			fprintf(out, "\n");
			doindent(out, indent);
			fprintf(out, "value=%s\n", val->b ? "true" : "false");
			break;
		case VT_CHAR:
			fprintf(out, "\n");
			doindent(out, indent);
			fprintf(out, "value=\\u%04"PRIX64, val->i);

			if (isprint(val->i))
				fprintf(out, " '%c'", (char) val->i);

			fprintf(out, "\n");
			break;
		case VT_CONS:
			fprintf(out, "\n");
			doindent(out, indent);
			fprintf(out, "head ");
			do_val_dump_file(out, val->cons.head, indent + 1);
			doindent(out, indent);
			fprintf(out, "tail ");
			do_val_dump_file(out, val->cons.tail, indent + 1);
			break;
		case VT_BLOB:
			fprintf(out, "\n");
			doindent(out, indent);
			fprintf(out, "ptr=%p size=%zu location=%s\n",
				val->blob.ptr, val->blob.size,
				val->static_alloc ? "static" : "heap");
			break;
		case VT_ARRAY: {
			size_t i;

			fprintf(out, " items=%zu\n", val->array.nelem);

			for (i = 0; i < val->array.nelem; i++) {
				doindent(out, indent);
				fprintf(out, "[%zu]: ", i);
				do_val_dump_file(out, val->array.vals[i],
						 indent + 1);
			}
			break;
		}
		case VT_NVL: {
			struct bst_tree *tree = &val->_set_nvl.values;
			struct nvpair *cur;

			fprintf(out, " items=%zu\n", bst_numnodes(tree));

			bst_for_each(tree, cur) {
				doindent(out, indent);
				fprintf(out, "name='%s' ", str_cstr(cur->name));
				do_val_dump_file(out, cur->value, indent + 1);
			}

			break;
		}
	}
}

void val_dump_file(FILE *out, struct val *val, int indent)
{
	doindent(out, indent);

	do_val_dump_file(out, val, indent + 1);
}
