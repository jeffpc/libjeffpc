/*
 * Copyright (c) 2015-2017 Josef 'Jeff' Sipek <jeffpc@josefsipek.net>
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

%option nounput
%option noinput
%option reentrant
%option bison-bridge
%option never-interactive

%{
#include "sexpr_impl.h"
#include "sexpr.tab.h"

extern void sexpr_error2(const char *e, const char *text);

#define YY_INPUT(buf, result, max_size)	\
	result = sexpr_reader_input_proc(buf, max_size, yyscanner)
int sexpr_reader_input_proc(char *buf, int size, yyscan_t scanner);

static void inc_lineno(yyscan_t scanner);

static int getint(const char *str, uint64_t *i, int base, const char *type,
		  int okretval)
{
	int ret;

	ret = __str2u64(str, i, base);
	if (ret) {
		char err[256];

		snprintf(err, sizeof(err), "base %d %s lexing error: %s",
			 base, type, xstrerror(ret));

		sexpr_error2(err, str);

		return ERROR;
	}

	return okretval;
}

static int getutf8(const char *str, size_t len, uint64_t *out)
{
	const uint8_t *tmp = (const uint8_t *) str;
	uint64_t c;

	/* process the first byte */
	c = *tmp;

	switch (len) {
		case 1:
			ASSERT3U(c, >=, 0x00);
			ASSERT3U(c, <=, 0x7f);
			c &= 0x7f;
			break;
		case 2:
			ASSERT3U(c, >=, 0xc0);
			ASSERT3U(c, <=, 0xdf);
			c &= 0x1f;
			break;
		case 3:
			ASSERT3U(c, >=, 0xe0);
			ASSERT3U(c, <=, 0xef);
			c &= 0x0f;
			break;
		case 4:
			ASSERT3U(c, >=, 0xf0);
			ASSERT3U(c, <=, 0xf7);
			c &= 0x07;
			break;
		default:
			return ERROR;
	}

	/* process remaining bytes */
	for (len--, tmp++; len; len--, tmp++) {
		uint64_t cur = *tmp;

		ASSERT3U(cur, >=, 0x80);
		ASSERT3U(cur, <=, 0xbf);

		c = (c << 6) | (*tmp & 0x3f);
	}

	*out = c;

	return CHAR;
}
%}

%x string

%%
	char string_buf[65536];
	char *string_ptr = NULL;

\"			{
				string_ptr = string_buf;
				BEGIN(string);
			}
<string>\"		{
				*string_ptr = '\0';
				yylval->s = STR_DUP(string_buf);
				BEGIN(INITIAL);
				return STRING;
			}
<string>[^\\"]*		{
				char *tmp = yytext;

				while (*tmp)
					*string_ptr++ = *tmp++;
			}
	/* TODO: handle octal escapes */
	/* TODO: handle hex escapes */
<string>\\n		{ *string_ptr++ = '\n'; }
<string>\\t		{ *string_ptr++ = '\t'; }
<string>\\r		{ *string_ptr++ = '\r'; }
<string>\\b		{ *string_ptr++ = '\b'; }
<string>\\f		{ *string_ptr++ = '\f'; }
<string>\\\\		{ *string_ptr++ = '\\'; }
<string>\\\"		{ *string_ptr++ = '"'; }

[().']			{ return *yytext; }
#[tf]			{ yylval->b = (yytext[1] == 't'); return BOOL; }
#n			{ return VNULL; }
0[0-7]*			{ return getint(yytext, &yylval->i, 8, "integer", NUMBER); }
0x[0-9a-fA-F]+		{ return getint(yytext, &yylval->i, 16, "integer", NUMBER); }
#x[0-9a-fA-F]+		{ return getint(yytext + 2, &yylval->i, 16, "integer", NUMBER); }
#\\u[0-9a-fA-F]+	{ return getint(yytext + 3, &yylval->i, 16, "character", CHAR); }

#\\nul			{ yylval->i = '\0'; return CHAR; }
#\\space		{ yylval->i = ' '; return CHAR; }

	/* #\<unicode char> */
#\\[\x00-\x7f]			{ return getutf8(yytext + 2, 1, &yylval->i); }
#\\[\x80-\xbf]			{ sexpr_error2("sexpr text contains illegal UTF-8 byte", yytext); yyterminate(); }
#\\[\xc0-\xdf][\x80-\xbf]	{ return getutf8(yytext + 2, 2, &yylval->i); }
#\\[\xe0-\xef][\x80-\xbf]{2}	{ return getutf8(yytext + 2, 3, &yylval->i); }
#\\[\xf0-\xf7][\x80-\xbf]{3}	{ return getutf8(yytext + 2, 4, &yylval->i); }
	/* TODO: handle #\<unicode char><combining char> */

[1-9][0-9]*		{ return getint(yytext, &yylval->i, 10, "integer", NUMBER); }
[a-zA-Z+=|&*?_-]?[a-zA-Z0-9+=|&*?_-]* { yylval->s = STR_DUP(yytext); return SYMBOL; }
[ \r\t]			{ /* ignore */ }
\n			{ inc_lineno(yyscanner); /* ignore */}
;[^\n]*			{ /* comment: ignore it */ }
.			{ sexpr_error2("sexp text contains invalid characters", yytext); yyterminate(); }
%%

static void inc_lineno(yyscan_t scanner)
{
	struct sexpr_parser_state *out;

	out = (struct sexpr_parser_state *) sexpr_reader_get_extra(scanner);

	out->lineno++;
}

int sexpr_reader_input_proc(char *buf, int size, yyscan_t scanner)
{
	struct sexpr_parser_state *out;
	int num;

	out = (struct sexpr_parser_state *) sexpr_reader_get_extra(scanner);
	num = out->len - out->pos;

	if (num <= 0)
		return 0;

	if (num > size)
		num = size;

	memcpy(buf, out->input + out->pos, num);
	out->pos += num;

	return num;
}

int sexpr_reader_wrap(yyscan_t scanner)
{
	return 1;
}
