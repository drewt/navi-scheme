/* Copyright 2014-2015 Drew Thoreson
 *
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/.
 */

#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <ctype.h>

#include "navi.h"
#include "compiler.h"
#include "navi/unicode.h"

/*
 * WARNING: What follows is a hand-rolled parser, and not a very good one.
 */

#define STR_BUF_LEN  64
#define STR_BUF_STEP 64

static int handle_case(struct navi_port *port, int c)
{
	return navi_port_is_fold_case(port) ? tolower(c) : c;
}

static inline void unexpected_eof(navi_env env)
{
	navi_read_error(env, "unexpected end of file");
}

static inline int peek_char(struct navi_port *port, navi_env env)
{
	navi_obj ch = navi_port_peek_char(port, env);
	if (navi_is_eof(ch))
		return EOF;
	return navi_char(ch);
}

static inline int ipeek_char(struct navi_port *port, navi_env env)
{
	int c = peek_char(port, env);
	if (c == EOF)
		unexpected_eof(env);
	return c;
}

static inline int read_char(struct navi_port *port, navi_env env)
{
	navi_obj ch = navi_port_read_char(port, env);
	if (navi_is_eof(ch))
		return EOF;
	return navi_char(ch);
}

static inline int iread_char(struct navi_port *port, navi_env env)
{
	int c = read_char(port, env);
	if (c == EOF)
		unexpected_eof(env);
	return c;
}

static inline int peek_first_char(struct navi_port *port, navi_env env)
{
	int c;
	while (isspace((c = peek_char(port, env))))
		read_char(port, env);
	return c;
}

static inline int ipeek_first_char(struct navi_port *port, navi_env env)
{
	int c = peek_first_char(port, env);
	if (c == EOF)
		unexpected_eof(env);
	return c;
}

static inline navi_obj navi_iread(struct navi_port *port, navi_env env)
{
	navi_obj expr = navi_read(port, env);
	if (navi_is_eof(expr))
		unexpected_eof(env);
	return expr;
}

static int isodigit(int c)
{
	return c >= '0' && c < '8';
}

static int isbdigit(int c)
{
	return c == '0' || c == '1';
}

static int ispipe(int c, navi_env env)
{
	if (c == EOF)
		unexpected_eof(env);
	return c == '|';
}

static int isterminal(int c, navi_env env)
{
	return isspace(c) || c == '(' || c == ')' || c == EOF;
}

static long decimal_value(char c, navi_env env)
{
	return c - '0';
}

static long hex_value(char c, navi_env env)
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
	navi_read_error(env, "invalid hex digit",
			navi_make_apair("digit", navi_make_char(c)));
}

static unsigned long read_unum(struct navi_port *port, int radix,
		int (*ctype)(int), long (*tonum)(char,navi_env), navi_env env)
{
	char c;
	unsigned long n = 0;
	while (ctype((c = peek_char(port, env)))) {
		n *= radix;
		n += tonum(c, env);
		read_char(port, env);
	}
	return n;
}

static long read_num(struct navi_port *port, int radix, int (*ctype)(int),
		long (*tonum)(char,navi_env), navi_env env)
{
	char c;
	long sign = 1;

	if ((c = peek_char(port, env)) == '-') {
		sign = -1;
		read_char(port, env);
	} else if (c == '+') {
		sign = 1;
		read_char(port, env);
	}
	return sign * read_unum(port, radix, ctype, tonum, env);
}

static navi_obj read_decimal(struct navi_port *port, navi_env env)
{
	return navi_make_fixnum(read_num(port, 10, isdigit, decimal_value, env));
}

static navi_obj read_hex(struct navi_port *port, navi_env env)
{
	return navi_make_fixnum(read_num(port, 16, isxdigit, hex_value, env));
}

static navi_obj read_octal(struct navi_port *port, navi_env env)
{
	return navi_make_fixnum(read_num(port, 8, isodigit, decimal_value, env));
}

static navi_obj read_binary(struct navi_port *port, navi_env env)
{
	return navi_make_fixnum(read_num(port, 2, isbdigit, decimal_value, env));
}

static char *read_until(struct navi_port *port, int(*ctype)(int,navi_env),
		navi_env env)
{
	UChar c;
	int32_t pos = 0;
	int32_t buf_len = STR_BUF_LEN;
	unsigned char *str = malloc(buf_len);
	if (!str)
		navi_enomem(env);
	while (!ctype((c = peek_char(port, env)), env)) {
		read_char(port, env);

		if (pos + u8_length(c) >= buf_len) {
			buf_len += STR_BUF_STEP;
			str = navi_critical_realloc(str, buf_len);
		}
		if (c > 0xFF) {
			u8_append(str, pos, buf_len, c);
		} else {
			str[pos++] = c;
		}
	}

	str[pos] = '\0';
	return (char*) str;
}

static UChar32 read_string_escape(struct navi_port *port, navi_env env)
{
	UChar32 c = iread_char(port, env);
	switch (c) {
	case 'a': return 0x7;
	case 'b': return 0x8;
	case 't': return 0x9;
	case 'n': return 0xA;
	case 'r': return 0xD;
	case '"': case '\\': case '|': return c;
	case ' ': case '\t': case '\n': case '\r':
	case '\f': case '\v':
		while (isspace((c = iread_char(port, env))))
			/* nothing */;
		return c;
	case 'x':
		c = read_unum(port, 16, isxdigit, hex_value, env);
		if (ipeek_char(port, env) != ';')
			navi_read_error(env, "missing terminator on string hex escape");
		read_char(port, env);
		return c;
	}
	navi_read_error(env, "unknown string escape", navi_make_apair("char", navi_make_char(c)));
}

static navi_obj read_string(struct navi_port *port, navi_env env)
{
	UChar32 c;
	int32_t pos = 0, length = 0;
	int32_t buf_len = STR_BUF_LEN;
	navi_obj obj = navi_make_string(buf_len, 0, 0);
	struct navi_string *str = navi_string(obj);

	while ((c = iread_char(port, env)) != '"') {
		if (c == '\\')
			c = read_string_escape(port, env);
		if (pos + u8_length(c) >= str->capacity)
			navi_string_grow_storage(str, STR_BUF_STEP);
		u8_append(str->data, pos, str->capacity, c);
		length++;
	}
	str->data[pos] = '\0';
	str->size = pos;
	str->length = length;
	return obj;
}

static inline navi_obj read_symbol(struct navi_port *port, int (*stop)(int,navi_env),
		navi_env env)
{
	char *str = read_until(port, stop, env);

	for (unsigned i = 0; str[i] != '\0'; i++)
		str[i] = handle_case(port, str[i]);
	return navi_make_symbol(str);
}

static navi_obj read_symbol_with_prefix(struct navi_port *port,
		int(*stop)(int,navi_env), int first, navi_env env)
{
	char *unfixed = read_until(port, stop, env);
	size_t length = strlen(unfixed);
	char *prefixed = malloc(length + 2);
	if (!prefixed)
		navi_enomem(env);
	prefixed[0] = first;
	for (size_t i = 0; i < length+1; i++)
		prefixed[i+1] = handle_case(port, unfixed[i]);

	return navi_make_symbol(prefixed);
}

static navi_obj read_character(struct navi_port *port, navi_env env)
{
	navi_obj ret;
	char *str = read_until(port, isterminal, env);
	size_t len = strlen(str);

	ret.n = 0;
	if (len == 1) {
		ret = navi_make_char(str[0]);
		goto end;
	}

	for (size_t i = 0; i < len; i++)
		str[i] = handle_case(port, str[i]);

	if (str[0] == 'x') {
		UChar32 ch = 0;
		for (int i = 1; str[i] != '\0'; i++) {
			if (!isxdigit(str[i]))
				break;
			ch *= 16;
			ch += hex_value(str[i], env);
		}
		if (!u_isdefined(ch))
			navi_read_error(env, "invalid unicode literal",
					navi_make_apair("value", navi_make_fixnum(ch)));
		ret = navi_make_char(ch);
		goto end;
	}

	#define named_char(name, ch) \
		if (!strcmp(str, name)) \
			ret = navi_make_char(ch);
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
		navi_read_error(env, "unknown named character",
				navi_make_apair("name", navi_cstr_to_string(str)));
end:
	free(str);
	return ret;
}

static navi_obj read_sharp(struct navi_port *port, navi_env env);

static void consume_line(struct navi_port *port, navi_env env)
{
	int c;
	while ((c = read_char(port, env)) != '\n' && c != EOF);
}

static navi_obj read_list(struct navi_port *port, navi_env env)
{
	struct navi_pair head, *elmptr;

	elmptr = &head;
	for (;;) {
		navi_obj expr;
		char c = ipeek_first_char(port, env);
		switch (c) {
		case ')':
			read_char(port, env);
			elmptr->cdr = navi_make_nil();
			return head.cdr;
		case '.':
			read_char(port, env);
			c = ipeek_char(port, env);
			if (!isterminal(c, env)) {
				// symbol beginning in .
				expr = read_symbol_with_prefix(port, isterminal, '.', env);
				break;
			}
			elmptr->cdr = navi_read(port, env);
			if ((c = peek_first_char(port, env)) != ')')
				navi_read_error(env, "missing list terminator");
			read_char(port, env);
			return head.cdr;
		case '#':
			read_char(port, env);
			if (navi_type((expr = read_sharp(port, env))) == NAVI_VOID)
				continue;
			break;
		case ';':
			consume_line(port, env);
			continue;
		default:
			expr = navi_iread(port, env);
		}
		elmptr->cdr = navi_make_empty_pair();
		elmptr = navi_pair(elmptr->cdr);
		elmptr->car = expr;
	}
	return head.cdr;
}

static navi_obj read_vector(struct navi_port *port, navi_env env)
{
	return navi_list_to_vector(read_list(port, env));
}

static navi_obj read_bytevec(struct navi_port *port, navi_env env)
{
	return navi_list_to_bytevec(read_list(port, env), env);
}

static navi_obj read_sharp_bang(struct navi_port *port, navi_env env)
{
	char *str = read_until(port, isterminal, env);
	if (str[0] == '/') {
		while (iread_char(port, env) != '\n');
		return navi_make_void();
	}
	#define read_syntax(name) if (!strcmp(str, name))
	if (str[0] == '\0' || str[0] == '/') {
		consume_line(port, env);
		return navi_make_void();
	}
	read_syntax("eof")
		return navi_make_eof();
	read_syntax("fold-case") {
		navi_port_set_fold_case(port, true);
		return navi_make_void();
	}
	read_syntax("no-fold-case") {
		navi_port_set_fold_case(port, false);
		return navi_make_void();
	}
	#undef read_syntax
	navi_read_error(env, "unknown shebang directive",
			navi_make_apair("directive", navi_cstr_to_string(str)));
}

static navi_obj read_boolean(struct navi_port *port, char fst, navi_env env)
{
	char buf[6] = { fst };
	char *str = read_until(port, isterminal, env);
	strncpy(buf+1, str, 4);
	free(str);
	if (!strcmp(buf, "t") || !strcmp(buf, "true"))
		return navi_make_bool(true);
	if (!strcmp(buf, "f") || !strcmp(buf, "false"))
		return navi_make_bool(false);
	navi_read_error(env, "invalid # syntax");
}

static navi_obj read_sharp(struct navi_port *port, navi_env env)
{
	char c, n;

	switch ((c = iread_char(port, env))) {
	case 't':
	case 'f':
		return read_boolean(port, c, env);
	case '\\':
		return read_character(port, env);
	case '(':
		return read_vector(port, env);
	case 'u':
		if ((n = iread_char(port, env)) != '8' ||
			(n = iread_char(port, env)) != '(')
			break;
		return read_bytevec(port, env);
	case 'b':
		return read_binary(port, env);
	case 'o':
		return read_octal(port, env);
	case 'x':
		return read_hex(port, env);
	case '!':
		return read_sharp_bang(port, env);
	case '#':
		return navi_get_internal(navi_read(port, env), env);
	case ';':
		navi_read(port, env);
		return navi_make_void();
	case '|':
		for (;;) {
			while (iread_char(port, env) != '|');
			if (iread_char(port, env) == '#')
				break;
		}
		return navi_make_void();
	case '/':
		while (iread_char(port, env) != '\n');
		return navi_make_void();

	}
	navi_read_error(env, "unknown disciminator",
			navi_make_apair("discriminator", navi_make_char(c)));
}

static navi_obj navi_sym_wrap(navi_obj symbol, navi_obj expr)
{
	return navi_make_pair(symbol, navi_make_pair(expr, navi_make_nil()));
}

navi_obj navi_read(struct navi_port *port, navi_env env)
{
	char c, d;
	int sign;
	navi_obj expr;

	switch ((c = peek_first_char(port, env))) {
	case '0': case '1': case '2': case '3': case '4':
	case '5': case '6': case '7': case '8': case '9':
		return read_decimal(port, env);
	case '"':
		read_char(port, env);
		return read_string(port, env);
	case '\'':
		read_char(port, env);
		return navi_sym_wrap(navi_sym_quote, navi_read(port, env));
	case '`':
		read_char(port, env);
		return navi_sym_wrap(navi_sym_quasiquote, navi_read(port, env));
	case ',':
		read_char(port, env);
		if ((c = ipeek_char(port, env)) == '@') {
			read_char(port, env);
			return navi_sym_wrap(navi_sym_splice, navi_read(port, env));
		}
		return navi_sym_wrap(navi_sym_unquote, navi_read(port, env));
	case '#':
		read_char(port, env);
		if (navi_type((expr = read_sharp(port, env))) == NAVI_VOID)
			return navi_read(port, env);
		return expr;
	case '(':
		read_char(port, env);
		return read_list(port, env);
	case ')':
		read_char(port, env);
		fprintf(stderr, "Unexpected character: %c\n", c);
		break;
	case ';':
		consume_line(port, env);
		return navi_read(port, env);
	case EOF:
		return navi_make_eof();
	case '|':
		read_char(port, env);
		expr = read_symbol(port, ispipe, env);
		read_char(port, env); /* consume closing pipe */
		return expr;
	case '+':
	case '-':
		read_char(port, env);
		sign = c == '+' ? 1 : -1;
		if (isdigit((d = peek_char(port, env)))) {
			expr = read_decimal(port, env);
			return navi_make_fixnum(navi_fixnum(expr) * sign);
		}
		return read_symbol_with_prefix(port, isterminal, c, env);
	default:
		return read_symbol(port, isterminal, env);
	}
	navi_read_error(env, "unexpected character",
			navi_make_apair("character", navi_make_char(c)));
}
