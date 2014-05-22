/* Copyright 2014 Drew Thoreson
 *
 * This program is free software: you can redistribute it and/or modify it
 * under the terms of the GNU General Public License as published by the Free
 * Software Foundation, version 2 of the License.
 *
 * This program is distributed in the hope that it will be useful, but WITHOUT
 * ANY WARRANTY; without even the implied warranty of MERCHANTABILITY or
 * FITNESS FOR A PARTICULAR PURPOSE.  See the GNU General Public License for
 * more details.
 *
 * You should have received a copy of the GNU General Public License along with
 * this program.  If not, see <http://www.gnu.org/licenses/>
 */

#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <ctype.h>

#include "sexp.h"

/*
 * WARNING: What follows is a hand-rolled parser, and not a very good one.
 *          Only one character of lookahead is used, since that is all the C
 *          standard guarantees for ungetc.
 */

#define STR_BUF_LEN  64
#define STR_BUF_STEP 64

#define fold_case tolower
static int no_fold_case(int c)
{
	return c;
}

static int (*handle_case)(int) = no_fold_case;

static inline void unexpected_eof(void)
{
	die("unexpected end of file");
}

static inline void sexp_ungetc(int c, FILE *stream)
{
	if (c != EOF)
		ungetc(c, stream);
}

static inline int igetchar(void)
{
	int c = getchar();
	if (c == EOF)
		unexpected_eof();
	return c;
}

static inline int first_char()
{
	int c;
	while (isspace((c = getchar())));
	return c;
}

static inline int ifirst_char()
{
	int c = first_char();
	if (c == EOF)
		unexpected_eof();
	return c;
}

static inline sexp_t sexp_iread()
{
	sexp_t sexp = sexp_read();
	if (is_eof(sexp))
		unexpected_eof();
	return sexp;
}

static int isodigit(int c)
{
	return c >= '0' && c < '8';
}

static int isbdigit(int c)
{
	return c == '0' || c == '1';
}

static int isquote(int c)
{
	if (c == EOF)
		unexpected_eof();
	return c == '"';
}

static int ispipe(int c)
{
	if (c == EOF)
		unexpected_eof();
	return c == '|';
}

static int isterminal(int c)
{
	return isspace(c) || c == '(' || c == ')' || c == EOF;
}

static long decimal_value(char c)
{
	return c - '0';
}

static long hex_value(char c)
{
	switch (c) {
	case '0': return 0;
	case '1': return 1;
	case '2': return 2;
	case '3': return 3;
	case '4': return 4;
	case '5': return 5;
	case '6': return 6;
	case '7': return 7;
	case '8': return 8;
	case '9': return 9;
	case 'a': case 'A': return 10;
	case 'b': case 'B': return 11;
	case 'c': case 'C': return 12;
	case 'd': case 'D': return 13;
	case 'e': case 'E': return 14;
	case 'f': case 'F': return 15;
	}
	die("invalid hex digit");
}

static sexp_t read_num(int radix, int (*ctype)(int), long (*tonum)(char))
{
	char c;
	long sign = 1, n = 0;

	if ((c = getchar()) == '-')
		sign = 0;
	else if (c == '+')
		sign = 1;
	else
		sexp_ungetc(c, stdin);

	while (ctype((c = getchar()))) {
		n *= radix;
		n += tonum(c);
	}
	sexp_ungetc(c, stdin);
	return make_num(n * sign);
}

static sexp_t read_decimal(void)
{
	return read_num(10, isdigit, decimal_value);
}

static sexp_t read_hex(void)
{
	return read_num(16, isxdigit, hex_value);
}

static sexp_t read_octal(void)
{
	return read_num(8, isodigit, decimal_value);
}

static sexp_t read_binary(void)
{
	return read_num(2, isbdigit, decimal_value);
}

static char *read_until(int(*ctype)(int), int first)
{
	char c = first;
	size_t pos = 0;
	size_t buf_len = STR_BUF_LEN;
	char *str = malloc(buf_len);

	if (first != -1) {
		if (ctype(first))
			goto end;
		str[pos++] = first;
	}

	while (!ctype((c = getchar()))) {
		str[pos++] = c;
		if (pos >= buf_len) {
			buf_len += STR_BUF_STEP;
			str = realloc(str, buf_len);
		}
	}
end:
	sexp_ungetc(c, stdin);
	str[pos] = '\0';
	return str;
}

static inline sexp_t read_string(void)
{
	sexp_t r;
	char *str = read_until(isquote, -1);
	getchar(); /* consume end-quote */
	r = to_string(str);
	free(str);
	return r;
}

static inline sexp_t read_symbol(int (*stop)(int), char first)
{
	char *str = read_until(stop, first);

	for (unsigned i = 0; str[i] != '\0'; i++)
		str[i] = handle_case(str[i]);
	return make_symbol(str);
}

static sexp_t read_character(void)
{
	sexp_t ret;
	char *str = read_until(isterminal, -1);
	size_t len = strlen(str);

	ret.n = 0;
	if (len == 1) {
		ret = make_char(str[0]);
		goto end;
	}

	for (size_t i = 0; i < len; i++)
		str[i] = handle_case(str[i]);

	if (str[0] == 'x') {
		uchar ch = 0;
		for (int i = 1; str[i] != '\0'; i++) {
			if (!isxdigit(str[i]))
				break;
			ch *= 16;
			ch += hex_value(str[i]);
		}
		if (!u_is_unicode(ch))
			die("invalid unicode literal");
		ret = make_char(ch);
		goto end;
	}

	#define named_char(name, ch) \
		if (!strcmp(str, name)) \
			ret = make_char(ch);
	named_char("alarm",     '\x7');
	named_char("backspace", '\x8');
	named_char("delete",    '\x7f');
	named_char("escape",    '\x1b');
	named_char("newline",   '\n');
	named_char("null",      '\0');
	named_char("return",    '\r');
	named_char("space",     ' ');
	named_char("tab",       '\t');
	#undef named_char

	if (ret.n == 0)
		die("unknown character: #\\%s\n", str);
end:
	free(str);
	return ret;
}

static sexp_t read_sharp(void);

static sexp_t read_list(void)
{
	struct sexp_pair head, *elmptr;

	elmptr = &head;
	for (;;) {
		sexp_t sexp;
		char c = ifirst_char();
		switch (c) {
		case ')':
			elmptr->cdr = make_nil();
			return head.cdr;
		case '.':
			elmptr->cdr = sexp_read();
			if ((c = first_char()) != ')')
				die("missing list terminator");
			return head.cdr;
		case '#':
			if (sexp_type((sexp = read_sharp())) == SEXP_VOID)
				continue;
			break;
		default:
			ungetc(c, stdin);
			sexp = sexp_iread();
		}
		elmptr->cdr = make_empty_pair();
		elmptr = &elmptr->cdr.p->data->pair;
		elmptr->car = sexp;
	}
	return head.cdr;
}

static sexp_t read_vector(void)
{
	return list_to_vector(read_list());
}

static sexp_t read_sharp_bang(void)
{
	char *str = read_until(isterminal, -1);
	#define read_syntax(name) if (!strcmp(str, name))
	read_syntax("eof")
		return make_eof();
	read_syntax("fold-case") {
		handle_case = fold_case;
		return make_void();
	}
	read_syntax("no-fold-case") {
		handle_case = no_fold_case;
		return make_void();
	}
	#undef read_syntax
	return make_void();
}

static sexp_t read_sharp(void)
{
	char c;

	switch ((c = igetchar())) {
	case 't':
		return make_bool(true);
	case 'f':
		return make_bool(false);
	case '\\':
		return read_character();
	case '(':
		return read_vector();
	case 'b':
		return read_binary();
	case 'o':
		return read_octal();
	case 'x':
		return read_hex();
	case '!':
		return read_sharp_bang();
	case ';':
		sexp_read();
		return make_void();
	case '|':
		for (;;) {
			while (igetchar() != '|');
			if (igetchar() == '#')
				break;
		}
		return make_void();

	}
	die("unknown literal: #%c", c); // TODO: error
}

static sexp_t sym_wrap(sexp_t symbol, sexp_t sexp)
{
	return make_pair(symbol, make_pair(sexp, make_nil()));
}

sexp_t sexp_read(void)
{
	char c, d;
	int sign;
	sexp_t sexp;

	switch ((c = first_char())) {
	case '0': case '1': case '2': case '3': case '4':
	case '5': case '6': case '7': case '8': case '9':
		ungetc(c, stdin);
		return read_decimal();
	case '"':
		return read_string();
	case '\'':
		return sym_wrap(sym_quote, sexp_read());
	case '`':
		return sym_wrap(sym_quasiquote, sexp_read());
	case ',':
		if ((c = igetchar()) == '@')
			return sym_wrap(sym_splice, sexp_read());
		ungetc(c, stdin);
		return sym_wrap(sym_unquote, sexp_read());
	case '#':
		if (sexp_type((sexp = read_sharp())) == SEXP_VOID)
			return sexp_read();
		return sexp;
	case '(':
		return read_list();
	case ')':
		fprintf(stderr, "Unexpected character: %c\n", c);
		break;
	case ';':
		while ((c = getchar()) != '\n' && c != EOF);
		return sexp_read();
	case EOF:
		return make_eof();
	case '|':
		sexp = read_symbol(ispipe, '\0');
		getchar(); /* consume closing pipe */
		return sexp;
	case '+':
	case '-':
		sign = c == '+' ? 1 : -1;
		if (isdigit((d = getchar()))) {
			ungetc(d, stdin);
			sexp = read_decimal();
			return make_num(sexp_num(sexp) * sign);
		}
		ungetc(d, stdin);
		return read_symbol(isterminal, c);
	default:
		return read_symbol(isterminal, c);
	}
	die("read error"); // TODO: error
}

DEFUN(scm_read, args)
{
	return sexp_read();
}
