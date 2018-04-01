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

void val_dump_file(FILE *out, struct val *val, int indent)
{
	if (!val)
		return;

	switch (val->type) {
		case VT_NULL:
			fprintf(out, "%*snull\n", indent, "");
			break;
		case VT_STR:
		case VT_SYM:
			fprintf(out, "%*s'%s'\n", indent, "", val_cstr(val));
			break;
		case VT_INT:
			fprintf(out, "%*s%"PRIu64"\n", indent, "", val->i);
			break;
		case VT_BOOL:
			fprintf(out, "%*s%s\n", indent, "",
				val->b ? "true" : "false");
			break;
		case VT_CHAR:
			if (isprint(val->i))
				fprintf(out, "%*s\\u%04"PRIX64": '%c'\n",
					indent, "", val->i, (char) val->i);
			else
				fprintf(out, "%*s\\u%04"PRIX64"\n", indent,
					"", val->i);
			break;
		case VT_CONS:
			fprintf(out, "%*scons head:\n", indent, "");
			val_dump(val->cons.head, indent + 2);
			fprintf(out, "%*scons tail:\n", indent, "");
			val_dump(val->cons.tail, indent + 2);
			break;
		case VT_BLOB:
			fprintf(out, "%*sblob @ %p.%zu (%s)\n", indent, "",
				val->blob.ptr, val->blob.size,
				val->static_alloc ? "static" : "heap");
			break;
		case VT_ARRAY: {
			size_t i;

			fprintf(out, "%*sarray[%zu]:\n", indent, "",
				val->array.nelem);

			for (i = 0; i < val->array.nelem; i++)
				val_dump_file(out, val->array.vals[i],
					      indent + 2);
			break;
		}
		case VT_NVL:
			/* TODO: dump the contents of the pairs */
			fprintf(out, "%*snvlist\n", indent, "");
			break;
		default:
			fprintf(out, "Unknown type %d\n", val->type);
			break;
	}
}
