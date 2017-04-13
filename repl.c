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

#include <libtecla.h>

#include <jeffpc/types.h>
#include <jeffpc/sexpr.h>

static char *repl_read(GetLine *gl)
{
	char *line;
	size_t len;

	line = gl_get_line(gl, "REPL> ", NULL, -1);
	if (!line)
		return NULL;

	len = strlen(line);
	if (line[len - 1] == '\n')
		line[len - 1] = '\0';

	return line;
}

static void repl_print(struct val *e)
{
	switch (e->type) {
		case VT_BOOL:
		case VT_INT:
		case VT_CHAR:
		case VT_STR:
			/* atoms get printed as-is */
			break;
		case VT_CONS:
		case VT_SYM:
			/* more complex things get a ' prepended */
			printf("'");
			break;
	}

	sexpr_dump_file(stdout, e, false);
	printf("\n");
}

int main(int argc, char **argv)
{
	GetLine *gl;

	gl = new_GetLine(1024, 1000);
	if (!gl) {
		fprintf(stderr, "Failed to initialize libtecla\n");
		return 1;
	}

	for (;;) {
		struct val *e;
		char *line;

		line = repl_read(gl);
		if (!line)
			break;

		if (strlen(line) == 0)
			continue;

		e = sexpr_parse(line, strlen(line));

		e = sexpr_eval(e, NULL);

		repl_print(e);

		val_putref(e);
	}

	del_GetLine(gl);

	return 0;
}
