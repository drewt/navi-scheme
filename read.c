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

static inline int peek_char(struct sexp_port *port)
{
	sexp_t ch = port_peek_char(port);
	if (is_eof(ch))
		return EOF;
	return sexp_char(ch);
}

static inline int ipeek_char(struct sexp_port *port)
{
	int c = peek_char(port);
	if (c == EOF)
		unexpected_eof();
	return c;
}

static inline int read_char(struct sexp_port *port)
{
	sexp_t ch = port_read_char(port);
	if (is_eof(ch))
		return EOF;
	return sexp_char(ch);
}

static inline int iread_char(struct sexp_port *port)
{
	int c = read_char(port);
	if (c == EOF)
		unexpected_eof();
	return c;
}

static inline int peek_first_char(struct sexp_port *port)
{
	int c;
	while (isspace((c = peek_char(port))))
		read_char(port);
	return c;
}

static inline int ipeek_first_char(struct sexp_port *port)
{
	int c = peek_first_char(port);
	if (c == EOF)
		unexpected_eof();
	return c;
}

static inline sexp_t sexp_iread(struct sexp_port *port)
{
	sexp_t sexp = sexp_read(port);
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

static sexp_t read_num(struct sexp_port *port, int radix, int (*ctype)(int),
		long (*tonum)(char))
{
	char c;
	long sign = 1, n = 0;

	if ((c = peek_char(port)) == '-') {
		sign = 0;
		read_char(port);
	} else if (c == '-') {
		sign = 1;
		read_char(port);
	}

	while (ctype((c = peek_char(port)))) {
		n *= radix;
		n += tonum(c);
		read_char(port);
	}
	return make_num(n * sign);
}

static sexp_t read_decimal(struct sexp_port *port)
{
	return read_num(port, 10, isdigit, decimal_value);
}

static sexp_t read_hex(struct sexp_port *port)
{
	return read_num(port, 16, isxdigit, hex_value);
}

static sexp_t read_octal(struct sexp_port *port)
{
	return read_num(port, 8, isodigit, decimal_value);
}

static sexp_t read_binary(struct sexp_port *port)
{
	return read_num(port, 2, isbdigit, decimal_value);
}

static char *read_until(struct sexp_port *port, int(*ctype)(int))
{
	char c;
	size_t pos = 0;
	size_t buf_len = STR_BUF_LEN;
	char *str = malloc(buf_len);

	while (!ctype((c = peek_char(port)))) {
		read_char(port);
		str[pos++] = c;
		if (pos >= buf_len) {
			buf_len += STR_BUF_STEP;
			str = realloc(str, buf_len);
		}
	}

	str[pos] = '\0';
	return str;
}

static inline sexp_t read_string(struct sexp_port *port)
{
	sexp_t r;
	char *str = read_until(port, isquote);
	read_char(port); /* consume end-quote */
	r = to_string(str);
	free(str);
	return r;
}

static inline sexp_t read_symbol(struct sexp_port *port, int (*stop)(int))
{
	char *str = read_until(port, stop);

	for (unsigned i = 0; str[i] != '\0'; i++)
		str[i] = handle_case(str[i]);
	return make_symbol(str);
}

static sexp_t read_symbol_with_prefix(struct sexp_port *port, int(*stop)(int),
		int first)
{
	char *unfixed = read_until(port, stop);
	size_t length = strlen(unfixed);
	char *prefixed = malloc(length + 2);

	prefixed[0] = first;
	for (size_t i = 0; i < length+1; i++)
		prefixed[i+1] = handle_case(unfixed[i]);

	return make_symbol(prefixed);
}

static sexp_t read_character(struct sexp_port *port)
{
	sexp_t ret;
	char *str = read_until(port, isterminal);
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

static sexp_t read_sharp(struct sexp_port *port);

static sexp_t read_list(struct sexp_port *port)
{
	struct sexp_pair head, *elmptr;

	elmptr = &head;
	for (;;) {
		sexp_t sexp;
		char c = ipeek_first_char(port);
		switch (c) {
		case ')':
			read_char(port);
			elmptr->cdr = make_nil();
			return head.cdr;
		case '.':
			read_char(port);
			elmptr->cdr = sexp_read(port);
			if ((c = peek_first_char(port)) != ')')
				die("missing list terminator");
			return head.cdr;
		case '#':
			read_char(port);
			if (sexp_type((sexp = read_sharp(port))) == SEXP_VOID)
				continue;
			break;
		default:
			sexp = sexp_iread(port);
		}
		elmptr->cdr = make_empty_pair();
		elmptr = &elmptr->cdr.p->data->pair;
		elmptr->car = sexp;
	}
	return head.cdr;
}

static sexp_t read_vector(struct sexp_port *port)
{
	return list_to_vector(read_list(port));
}

static sexp_t read_sharp_bang(struct sexp_port *port)
{
	char *str = read_until(port, isterminal);
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

static sexp_t read_sharp(struct sexp_port *port)
{
	char c;

	switch ((c = iread_char(port))) {
	case 't':
		return make_bool(true);
	case 'f':
		return make_bool(false);
	case '\\':
		return read_character(port);
	case '(':
		return read_vector(port);
	case 'b':
		return read_binary(port);
	case 'o':
		return read_octal(port);
	case 'x':
		return read_hex(port);
	case '!':
		return read_sharp_bang(port);
	case ';':
		sexp_read(port);
		return make_void();
	case '|':
		for (;;) {
			while (iread_char(port) != '|');
			if (iread_char(port) == '#')
				break;
		}
		return make_void();
	case '/':
		while (iread_char(port) != '\n');
		return make_void();

	}
	die("unknown literal: #%c", c); // TODO: error
}

static sexp_t sym_wrap(sexp_t symbol, sexp_t sexp)
{
	return make_pair(symbol, make_pair(sexp, make_nil()));
}

sexp_t sexp_read(struct sexp_port *port)
{
	char c, d;
	int sign;
	sexp_t sexp;

	switch ((c = peek_first_char(port))) {
	case '0': case '1': case '2': case '3': case '4':
	case '5': case '6': case '7': case '8': case '9':
		return read_decimal(port);
	case '"':
		read_char(port);
		return read_string(port);
	case '\'':
		read_char(port);
		return sym_wrap(sym_quote, sexp_read(port));
	case '`':
		read_char(port);
		return sym_wrap(sym_quasiquote, sexp_read(port));
	case ',':
		read_char(port);
		if ((c = ipeek_char(port)) == '@') {
			read_char(port);
			return sym_wrap(sym_splice, sexp_read(port));
		}
		return sym_wrap(sym_unquote, sexp_read(port));
	case '#':
		read_char(port);
		if (sexp_type((sexp = read_sharp(port))) == SEXP_VOID)
			return sexp_read(port);
		return sexp;
	case '(':
		read_char(port);
		return read_list(port);
	case ')':
		read_char(port);
		fprintf(stderr, "Unexpected character: %c\n", c);
		break;
	case ';':
		while ((c = read_char(port)) != '\n' && c != EOF);
		return sexp_read(port);
	case EOF:
		return make_eof();
	case '|':
		read_char(port);
		sexp = read_symbol(port, ispipe);
		read_char(port); /* consume closing pipe */
		return sexp;
	case '+':
	case '-':
		read_char(port);
		sign = c == '+' ? 1 : -1;
		if (isdigit((d = peek_char(port)))) {
			sexp = read_decimal(port);
			return make_num(sexp_num(sexp) * sign);
		}
		return read_symbol_with_prefix(port, isterminal, c);
	default:
		return read_symbol(port, isterminal);
	}
	die("read error"); // TODO: error
}
