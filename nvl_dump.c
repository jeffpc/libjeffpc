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
#include <jeffpc/nvl.h>
#include <jeffpc/thread.h>

#define INDENT	4

static void __dump_nvl(FILE *out, struct nvlist *nvl, int indent);

static const char *typename(int type)
{
	static const char *typenames[] = {
		[NVT_ARRAY] = "array",
		[NVT_BLOB] = "blob",
		[NVT_BOOL] = "bool",
		[NVT_INT] = "int",
		[NVT_NVL] = "nvlist",
		[NVT_NULL] = "null",
		[NVT_STR] = "string",
	};

	static __thread char badname[10];

	if ((type < 0) || (type > NVT_STR))
		goto bad;

	if (typenames[type] == NULL)
		goto bad;

	return typenames[type];

bad:
	snprintf(badname, sizeof(badname), "<%d>", type);
	return badname;
}

static inline void doindent(FILE *out, int howmuch)
{
	fprintf(out, "%*s", INDENT * howmuch, "");
}

static void __dump_val(FILE *out, const struct nvval *val, int indent)
{
	size_t i;

	fprintf(out, " type=%s", typename(val->type));

	switch (val->type) {
		case NVT_ARRAY:
			fprintf(out, " nelem=%zu\n", val->array.nelem);
			for (i = 0; i < val->array.nelem; i++) {
				doindent(out, indent);
				fprintf(out, "[%zu]: ", i);
				__dump_val(out, &val->array.vals[i],
					   indent + 1);
			}
			break;
		case NVT_BLOB:
			fprintf(out, " size=%zu\n", val->blob.size);
			break;
		case NVT_BOOL:
			fprintf(out, "\n");
			doindent(out, indent);
			fprintf(out, "value=%s\n", val->b ? "true" : "false");
			break;
		case NVT_INT:
			fprintf(out, "\n");
			doindent(out, indent);
			fprintf(out, "value=%"PRIu64"\n", val->i);
			break;
		case NVT_NULL:
			fprintf(out, "\n");
			break;
		case NVT_NVL:
			fprintf(out, "\n");
			__dump_nvl(out, val->nvl, indent);
			break;
		case NVT_STR:
			fprintf(out, "\n");
			doindent(out, indent);
			fprintf(out, "value='%s'\n", str_cstr(val->str));
			break;
	}
}

static void __dump_nvl(FILE *out, struct nvlist *nvl, int indent)
{
	const struct nvpair *pair;

	nvl_for_each(pair, nvl) {
		doindent(out, indent);
		fprintf(out, "name='%s'", pair->name);
		__dump_val(out, &pair->value, indent + 1);
	}
}

void nvl_dump_file(FILE *out, struct nvlist *nvl)
{
	fprintf(out, "nvlist dump @ %p\n", nvl);
	__dump_nvl(out, nvl, 1);
}
