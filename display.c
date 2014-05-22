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

#include <stdio.h>

#include "sexp.h"

static void display_cdr(sexp_t cdr, bool head, bool write)
{
	switch (sexp_type(cdr)) {
	case SEXP_PAIR:
		if (!head)
			putchar(' ');
		_display(sexp_pair(cdr)->car, write);
		display_cdr(sexp_pair(cdr)->cdr, false, write);
		break;
	case SEXP_NIL:
		putchar(')');
		break;
	default:
		printf(" . ");
		_display(cdr, write);
		putchar(')');
		break;
	}
}

static void display_vector(sexp_t sexp, bool write)
{
	struct sexp_vector *vec = sexp_vector(sexp);

	if (vec->size == 0) {
		printf("#()");
		return;
	}

	printf("#(");
	_display(vec->data[0], write);

	for (size_t i = 1; i < vec->size; i++) {
		printf(", ");
		_display(vec->data[i], write);
	}

	putchar(')');
}

static void display_bytevec(sexp_t sexp)
{
	struct sexp_bytevec *vec = sexp_bytevec(sexp);

	if (vec->size == 0) {
		printf("#u8()");
		return;
	}

	printf("#u8(%u", vec->data[0]);

	for (size_t i = 1; i < vec->size; i++)
		printf(", %u", vec->data[i]);

	putchar(')');
}

static inline void display_string(sexp_t string)
{
	struct sexp_bytevec *vec = sexp_bytevec(string);
	for (size_t i = 0; i < vec->size; i++)
		putchar(vec->data[i]);
}

void _display(sexp_t sexp, bool write)
{
	switch (sexp_type(sexp)) {
	case SEXP_VOID:
		break;
	case SEXP_NIL:
		printf("()");
		break;
	case SEXP_EOF:
		printf("#!eof");
		break;
	case SEXP_NUM:
		printf("%ld", sexp_num(sexp));
		break;
	case SEXP_BOOL:
		printf("#%c", sexp_bool(sexp) ? 't' : 'f');
		break;
	case SEXP_CHAR:
		if (sexp_char(sexp) > 127)
			printf("#\\x%lx", sexp_char(sexp));
		else
			printf("#\\%c", (char) sexp_char(sexp));
		break;
	case SEXP_VALUES:
		printf("<values ");
		display_vector(sexp, write);
		putchar('>');
		break;
	case SEXP_PAIR:
		putchar('(');
		display_cdr(sexp, true, write);
		break;
	case SEXP_STRING:
		printf(write ? "\"%s\"" : "%s", sexp_bytevec(sexp)->data);
		break;
	case SEXP_SYMBOL:
		display_string(sexp);
		break;
	case SEXP_VECTOR:
		display_vector(sexp, write);
		break;
	case SEXP_BYTEVEC:
		display_bytevec(sexp);
		break;
	case SEXP_MACRO:
		printf("#<macro %s>", sexp_fun(sexp)->name);
		break;
	case SEXP_FUNCTION:
		if (sexp_fun(sexp)->builtin)
			printf("#<builtin-procedure %s>", sexp_fun(sexp)->name);
		else
			printf("#<interpreted-procedure %s>", sexp_fun(sexp)->name);
		break;
	case SEXP_CASELAMBDA:
		printf("#<case-lambda>");
		break;
	case SEXP_ESCAPE:
		printf("#<escape continuation>");
		break;
	case SEXP_ENVIRONMENT:
		printf("#<environment>");
		break;
	case SEXP_BOUNCE:
		printf("#<bounce>");
		break;
	}
}

DEFUN(scm_display, args)
{
	display(sexp_pair(args)->car);
	return unspecified();
}

DEFUN(scm_write, args)
{
	sexp_write(car(args));
	return unspecified();
}

DEFUN(scm_newline, args)
{
	putchar('\n');
	return unspecified();
}
