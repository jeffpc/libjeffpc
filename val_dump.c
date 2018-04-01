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

static inline void doindent(FILE *out, int indent)
{
	fprintf(out, "%*s", indent, "");
}

static void do_val_dump_file(FILE *out, struct val *val, int indent,
			     bool indented)
{
	if (!val)
		return;

	if (!indented)
		doindent(out, indent);

	switch (val->type) {
		case VT_NULL:
			fprintf(out, "null\n");
			break;
		case VT_STR:
		case VT_SYM:
			fprintf(out, "'%s' (%s)\n", val_cstr(val),
				(val->type == VT_STR) ? "str" : "sym");
			break;
		case VT_INT:
			fprintf(out, "%"PRIu64"\n", val->i);
			break;
		case VT_BOOL:
			fprintf(out, "%s\n", val->b ? "true" : "false");
			break;
		case VT_CHAR:
			if (isprint(val->i))
				fprintf(out, "\\u%04"PRIX64": '%c'\n",
					val->i, (char) val->i);
			else
				fprintf(out, "\\u%04"PRIX64"\n", val->i);
			break;
		case VT_CONS:
			fprintf(out, "cons head:\n");
			do_val_dump_file(out, val->cons.head, indent + 2, false);
			doindent(out, indent);
			fprintf(out, "cons tail:\n");
			do_val_dump_file(out, val->cons.tail, indent + 2, false);
			break;
		case VT_BLOB:
			fprintf(out, "blob @ %p.%zu (%s)\n",
				val->blob.ptr, val->blob.size,
				val->static_alloc ? "static" : "heap");
			break;
		case VT_ARRAY: {
			size_t i;

			fprintf(out, "array[%zu]:\n", val->array.nelem);

			for (i = 0; i < val->array.nelem; i++) {
				doindent(out, indent + 2);
				fprintf(out, "[%zu]: ", i);
				do_val_dump_file(out, val->array.vals[i],
						 indent + 2, true);
			}
			break;
		}
		case VT_NVL: {
			struct bst_tree *tree = &val->_set_nvl.values;
			struct nvpair *cur;

			fprintf(out, "nvlist[%zu]:\n", bst_numnodes(tree));

			bst_for_each(tree, cur) {
				doindent(out, indent + 2);
				fprintf(out, "['%s']: ", str_cstr(cur->name));
				do_val_dump_file(out, cur->value, indent + 2,
						 true);
			}

			break;
		}
		default:
			fprintf(out, "Unknown type %d\n", val->type);
			break;
	}
}

void val_dump_file(FILE *out, struct val *val, int indent)
{
	do_val_dump_file(out, val, indent, false);
}
