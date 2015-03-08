/* Copyright 2014-2015 Drew Thoreson
 *
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/.
 */

#include <stdio.h>

#include "navi.h"

static void write_void(struct navi_port *p, navi_obj o, bool w, navi_env env)
{
	/* nothing */
}

static void write_fixnum(struct navi_port *p, navi_obj o, bool w, navi_env env)
{
	char buf[128];
	snprintf(buf, 128, "%ld", navi_fixnum(o));
	buf[127] = '\0';
	navi_port_write_cstr(buf, p, env);
}

static void write_bool(struct navi_port *p, navi_obj o, bool w, navi_env env)
{
	char buf[3] = { '#', '?', '\0' };
	buf[1] = navi_bool(o) ? 't' : 'f';
	navi_port_write_cstr(buf, p, env);
}

static void write_char(struct navi_port *p, navi_obj o, bool w, navi_env env)
{
	char buf[128];
	if (navi_char(o) > 127)
		snprintf(buf, 128, "#\\x%lx", navi_char(o));
	else
		snprintf(buf, 128, "#\\%c", (char)navi_char(o));
	buf[127] = '\0';
	navi_port_write_cstr(buf, p, env);
}

static void write_cdr(struct navi_port *port, navi_obj cdr, bool head,
		bool write, navi_env env)
{
	switch (navi_type(cdr)) {
	case NAVI_PAIR:
		if (!head) navi_port_write_cstr(" ", port, env);
		_navi_display(port, navi_pair(cdr)->car, write, env);
		write_cdr(port, navi_pair(cdr)->cdr, false, write, env);
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

static void write_pair(struct navi_port *p, navi_obj o, bool w, navi_env env)
{
	navi_port_write_cstr("(", p, env);
	write_cdr(p, o, true, w, env);
}

static void write_string(struct navi_port *p, navi_obj o, bool w, navi_env env)
{
	if (w)
		navi_port_write_cstr("\"", p, env);
	navi_port_write_cstr((char*)navi_string(o)->data, p, env);
	if (w)
		navi_port_write_cstr("\"", p, env);
}

static void write_symbol(struct navi_port *p, navi_obj o, bool w, navi_env env)
{
	char *ptr;
	struct navi_symbol *sym = navi_symbol(o);
	for (ptr = sym->data; *ptr != '\0'; ptr++)
		navi_port_write_byte(*ptr, p, env);
}

static void write_vector(struct navi_port *p, navi_obj o, bool w, navi_env env)
{
	struct navi_vector *vec = navi_vector(o);

	if (vec->size == 0) {
		navi_port_write_cstr("#()", p, env);
		return;
	}

	navi_port_write_cstr("#(", p, env);
	_navi_display(p, vec->data[0], w, env);

	for (size_t i = 1; i < vec->size; i++) {
		navi_port_write_cstr(" ", p, env);
		_navi_display(p, vec->data[i], w, env);
	}

	navi_port_write_cstr(")", p, env);
}

static void write_bytevec(struct navi_port *p, navi_obj o, bool w, navi_env env)
{
	struct navi_bytevec *vec = navi_bytevec(o);

	if (vec->size == 0) {
		navi_port_write_cstr("#u8(", p, env);
		return;
	}

	navi_port_write_cstr("#u8(", p, env);
	_navi_display(p, navi_make_fixnum(vec->data[0]), w, env);

	for (size_t i = 1; i < vec->size; i++) {
		navi_port_write_cstr(" ", p, env);
		_navi_display(p, navi_make_fixnum(vec->data[i]), w, env);
	}

	navi_port_write_cstr(")", p, env);
}

static void write_procedure(struct navi_port *p, navi_obj o, bool w, navi_env env)
{
	if (navi_is_builtin(o))
		navi_port_write_cstr("#<builtin-procedure ", p, env);
	else
		navi_port_write_cstr("#<interpreted-procedure ", p, env);
	write_symbol(p, navi_procedure(o)->name, w, env);
	navi_port_write_cstr(">", p, env);
}

static void write_thunk(struct navi_port *p, navi_obj o, bool w, navi_env env)
{
	navi_port_write_cstr("#<thunk ", p, env);
	_navi_display(p, navi_thunk(o)->expr, w, env);
	navi_port_write_cstr(">", p, env);
}

#define SIMPLE_WRITE(name, str) \
	static void name(struct navi_port *p, navi_obj o, bool w, navi_env env) \
	{ \
		navi_port_write_cstr(str, p, env); \
	}

SIMPLE_WRITE(write_nil, "()")
SIMPLE_WRITE(write_eof, "#!eof")
SIMPLE_WRITE(write_port, "#<port>")
SIMPLE_WRITE(write_promise, "#<promise>")
SIMPLE_WRITE(write_caselambda, "#<case-lambda>")
SIMPLE_WRITE(write_escape, "#<escape continuation>")
SIMPLE_WRITE(write_environment, "#<environment>")
SIMPLE_WRITE(write_bounce, "#<bounce>")

#define WRITE_WITH_CALL(name, tag, arg) \
	static void name(struct navi_port *p, navi_obj o, bool w, navi_env env) \
	{ \
		navi_port_write_cstr("#<" tag " ", p, env); \
		arg; \
		navi_port_write_cstr(">", p, env); \
	}

WRITE_WITH_CALL(write_values, "values", write_vector(p, o, w, env))
WRITE_WITH_CALL(write_macro, "macro",
		write_symbol(p, navi_procedure(o)->name, w, env))
WRITE_WITH_CALL(write_special, "special form",
		write_symbol(p, navi_procedure(o)->name, w, env))
WRITE_WITH_CALL(write_parameter, "parameter",
		write_symbol(p, navi_parameter_key(o), w, env))

typedef void (*write_fn)(struct navi_port*, navi_obj, bool, navi_env);

static const write_fn writetab[] = {
	[NAVI_VOID]        = write_void,
	[NAVI_NIL]         = write_nil,
	[NAVI_EOF]         = write_eof,
	[NAVI_FIXNUM]      = write_fixnum,
	[NAVI_BOOL]        = write_bool,
	[NAVI_CHAR]        = write_char,
	[NAVI_VALUES]      = write_values,
	[NAVI_PAIR]        = write_pair,
	[NAVI_PORT]        = write_port,
	[NAVI_STRING]      = write_string,
	[NAVI_SYMBOL]      = write_symbol,
	[NAVI_VECTOR]      = write_vector,
	[NAVI_BYTEVEC]     = write_bytevec,
	[NAVI_THUNK]       = write_thunk,
	[NAVI_MACRO]       = write_macro,
	[NAVI_SPECIAL]     = write_special,
	[NAVI_PROCEDURE]   = write_procedure,
	[NAVI_PROMISE]     = write_promise,
	[NAVI_CASELAMBDA]  = write_caselambda,
	[NAVI_ESCAPE]      = write_escape,
	[NAVI_PARAMETER]   = write_parameter,
	[NAVI_ENVIRONMENT] = write_environment,
	[NAVI_BOUNCE]      = write_bounce,
};

void _navi_display(struct navi_port *port, navi_obj obj, bool write, navi_env env)
{
	assert(navi_type(obj) < sizeof(writetab)/sizeof(*writetab));
	writetab[navi_type(obj)](port, obj, write, env);
}
