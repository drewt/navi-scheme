/* Copyright 2014-2015 Drew Thoreson
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

#include "navi.h"

static void display_cdr(struct navi_port *port, navi_obj cdr, bool head,
		bool write, navi_env env)
{
	switch (navi_type(cdr)) {
	case NAVI_PAIR:
		if (!head) navi_port_write_cstr(" ", port, env);
		_navi_display(port, navi_pair(cdr)->car, write, env);
		display_cdr(port, navi_pair(cdr)->cdr, false, write, env);
		break;
	case NAVI_NIL:
		navi_port_write_cstr(")", port, env);
		break;
	default:
		navi_port_write_cstr(" . ", port, env);
		_navi_display(port, cdr, write, env);
		navi_port_write_cstr(")", port, env);
		break;
	}
}

static void display_vector(struct navi_port *port, navi_obj obj, bool write,
		navi_env env)
{
	struct navi_vector *vec = navi_vector(obj);

	if (vec->size == 0) {
		navi_port_write_cstr("#()", port, env);
		return;
	}

	navi_port_write_cstr("#(", port, env);
	_navi_display(port, vec->data[0], write, env);

	for (size_t i = 1; i < vec->size; i++) {
		navi_port_write_cstr(" ", port, env);
		_navi_display(port, vec->data[i], write, env);
	}

	navi_port_write_cstr(")", port, env);
}

static void display_bytevec(struct navi_port *port, navi_obj obj, navi_env env)
{
	struct navi_bytevec *vec = navi_bytevec(obj);

	if (vec->size == 0) {
		navi_port_write_cstr("#u8(", port, env);
		return;
	}

	navi_port_write_cstr("#u8(", port, env);
	_navi_display(port, navi_make_num(vec->data[0]), false, env);

	for (size_t i = 1; i < vec->size; i++) {
		navi_port_write_cstr(" ", port, env);
		_navi_display(port, navi_make_num(vec->data[i]), false, env);
	}

	navi_port_write_cstr(")", port, env);
}

static void display_symbol(struct navi_port *port, navi_obj obj, navi_env env)
{
	char *p;
	struct navi_symbol *sym = navi_symbol(obj);
	for (p = sym->data; *p != '\0'; p++)
		navi_port_write_byte(*p, port, env);
}

void _navi_display(struct navi_port *port, navi_obj obj, bool write, navi_env env)
{
	char buf[128];
	switch (navi_type(obj)) {
	case NAVI_VOID:
		break;
	case NAVI_NIL:
		navi_port_write_cstr("()", port, env);
		break;
	case NAVI_EOF:
		navi_port_write_cstr("#!eof", port, env);
		break;
	case NAVI_NUM:
		snprintf(buf, 128, "%ld", navi_num(obj));
		buf[127] = '\0';
		navi_port_write_cstr(buf, port, env);
		break;
	case NAVI_BOOL:
		navi_port_write_cstr("#", port, env);
		navi_port_write_cstr(navi_bool(obj) ? "t" : "f", port, env);
		break;
	case NAVI_CHAR:
		if (navi_char(obj) > 127) {
			snprintf(buf, 128, "#\\x%lx", navi_char(obj));
			buf[127] = '\0';
			navi_port_write_cstr(buf, port, env);
		} else {
			navi_port_write_cstr("#\\", port, env);
			navi_port_write_char(navi_char(obj), port, env);
		}
		break;
	case NAVI_VALUES:
		navi_port_write_cstr("#<values ", port, env);
		display_vector(port, obj, write, env);
		navi_port_write_cstr(">", port, env);
		break;
	case NAVI_PAIR:
		navi_port_write_cstr("(", port, env);
		display_cdr(port, obj, true, write, env);
		break;
	case NAVI_PORT:
		navi_port_write_cstr("#<port>", port, env);
		break;
	case NAVI_STRING:
		if (write) navi_port_write_cstr("\"", port, env);
		navi_port_write_cstr((char*)navi_string(obj)->data, port, env);
		if (write) navi_port_write_cstr("\"", port, env);
		break;
	case NAVI_SYMBOL:
		display_symbol(port, obj, env);
		break;
	case NAVI_VECTOR:
		display_vector(port, obj, write, env);
		break;
	case NAVI_BYTEVEC:
		display_bytevec(port, obj, env);
		break;
	case NAVI_MACRO:
		navi_port_write_cstr("#<macro ", port, env);
		display_symbol(port, navi_procedure(obj)->name, env);
		navi_port_write_cstr(">", port, env);
		break;
	case NAVI_SPECIAL:
		navi_port_write_cstr("#<special form ", port, env);
		display_symbol(port, navi_procedure(obj)->name, env);
		navi_port_write_cstr(">", port, env);
		break;
	case NAVI_PROMISE:
		navi_port_write_cstr("#<promise>", port, env);
		break;
	case NAVI_PROCEDURE:
		if (navi_is_builtin(obj)) {
			navi_port_write_cstr("#<builtin-procedure ", port, env);
			display_symbol(port, navi_procedure(obj)->name, env);
			navi_port_write_cstr(">", port, env);
		} else {
			navi_port_write_cstr("#<interpreted-procedure ", port, env);
			display_symbol(port, navi_procedure(obj)->name, env);
			navi_port_write_cstr(">", port, env);
		}
		break;
	case NAVI_CASELAMBDA:
		navi_port_write_cstr("#<case-lambda>", port, env);
		break;
	case NAVI_ESCAPE:
		navi_port_write_cstr("#<escape continuation>", port, env);
		break;
	case NAVI_PARAMETER:
		navi_port_write_cstr("#<parameter ", port, env);
		display_symbol(port, navi_parameter_key(obj), env);
		navi_port_write_cstr(">", port, env);
		break;
	case NAVI_ENVIRONMENT:
		navi_port_write_cstr("#<environment>", port, env);
		break;
	case NAVI_BOUNCE:
		navi_port_write_cstr("#<bounce>", port, env);
		break;
	}
}
