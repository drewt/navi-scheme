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

static void display_cdr(struct sexp_port *port, sexp_t cdr, bool head,
		bool write)
{
	switch (sexp_type(cdr)) {
	case SEXP_PAIR:
		if (!head) port_write_c_string(" ", port);
		_display(port, sexp_pair(cdr)->car, write);
		display_cdr(port, sexp_pair(cdr)->cdr, false, write);
		break;
	case SEXP_NIL:
		port_write_c_string(")", port);
		break;
	default:
		port_write_c_string(" . ", port);
		_display(port, cdr, write);
		port_write_c_string(")", port);
		break;
	}
}

static void display_vector(struct sexp_port *port, sexp_t sexp, bool write)
{
	struct sexp_vector *vec = sexp_vector(sexp);

	if (vec->size == 0) {
		port_write_c_string("#()", port);
		return;
	}

	port_write_c_string("#(", port);
	_display(port, vec->data[0], write);

	for (size_t i = 1; i < vec->size; i++) {
		port_write_c_string(" ", port);
		_display(port, vec->data[i], write);
	}

	port_write_c_string(")", port);
}

static void display_bytevec(struct sexp_port *port, sexp_t sexp)
{
	struct sexp_bytevec *vec = sexp_bytevec(sexp);

	if (vec->size == 0) {
		port_write_c_string("#u8(", port);
		return;
	}

	port_write_c_string("#u8(", port);
	_display(port, make_num(vec->data[0]), false);

	for (size_t i = 1; i < vec->size; i++) {
		port_write_c_string(" ", port);
		_display(port, make_num(vec->data[i]), false);
	}

	port_write_c_string(")", port);
}

static void display_symbol(struct sexp_port *port, sexp_t sexp)
{
	struct sexp_bytevec *vec = sexp_bytevec(sexp);
	for (size_t i = 0; i < vec->size; i++)
		port_write_char(make_char(vec->data[i]), port);
}

void _display(struct sexp_port *port, sexp_t sexp, bool write)
{
	char buf[128];
	switch (sexp_type(sexp)) {
	case SEXP_VOID:
		break;
	case SEXP_NIL:
		port_write_c_string("()", port);
		break;
	case SEXP_EOF:
		port_write_c_string("#!eof", port);
		break;
	case SEXP_NUM:
		snprintf(buf, 128, "%ld", sexp_num(sexp));
		buf[127] = '\0';
		port_write_c_string(buf, port);
		break;
	case SEXP_BOOL:
		port_write_c_string("#", port);
		port_write_c_string(sexp_bool(sexp) ? "t" : "f", port);
		break;
	case SEXP_CHAR:
		if (sexp_char(sexp) > 127) {
			snprintf(buf, 128, "#\\x%lx", sexp_char(sexp));
			buf[127] = '\0';
			port_write_c_string(buf, port);
		} else {
			port_write_c_string("#\\", port);
			port_write_char(sexp, port);
		}
		break;
	case SEXP_VALUES:
		port_write_c_string("#<values ", port);
		display_vector(port, sexp, write);
		port_write_c_string(">", port);
		break;
	case SEXP_PAIR:
		port_write_c_string("(", port);
		display_cdr(port, sexp, true, write);
		break;
	case SEXP_PORT:
		port_write_c_string("#<port>", port);
		break;
	case SEXP_STRING:
		if (write) port_write_c_string("\"", port);
		port_write_c_string(sexp_string(sexp)->data, port);
		if (write) port_write_c_string("\"", port);
		break;
	case SEXP_SYMBOL:
		display_symbol(port, sexp);
		break;
	case SEXP_VECTOR:
		display_vector(port, sexp, write);
		break;
	case SEXP_BYTEVEC:
		display_bytevec(port, sexp);
		break;
	case SEXP_MACRO:
		port_write_c_string("#<macro ", port);
		port_write_c_string(sexp_fun(sexp)->name, port);
		port_write_c_string(">", port);
		break;
	case SEXP_SPECIAL:
		port_write_c_string("#<special form ", port);
		port_write_c_string(sexp_fun(sexp)->name, port);
		port_write_c_string(">", port);
		break;
	case SEXP_FUNCTION:
		if (sexp_fun(sexp)->builtin) {
			port_write_c_string("#<builtin-procedure ", port);
			port_write_c_string(sexp_fun(sexp)->name, port);
			port_write_c_string(">", port);
		} else {
			port_write_c_string("#<interpreted-procedure ", port);
			port_write_c_string(sexp_fun(sexp)->name, port);
			port_write_c_string(">", port);
		}
		break;
	case SEXP_CASELAMBDA:
		port_write_c_string("#<case-lambda>", port);
		break;
	case SEXP_ESCAPE:
		port_write_c_string("#<escape continuation>", port);
		break;
	case SEXP_ENVIRONMENT:
		port_write_c_string("#<environment>", port);
		break;
	case SEXP_BOUNCE:
		port_write_c_string("#<bounce>", port);
		break;
	}
}
