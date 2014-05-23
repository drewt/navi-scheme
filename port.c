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

#include "sexp.h"

static sexp_t stdio_read(struct sexp_port *port)
{
	return make_char(getc(port->specific));
}

static void stdio_write(sexp_t ch, struct sexp_port *port)
{
	putc(sexp_char(ch), port->specific);
}

sexp_t make_stdio_port(FILE *stream)
{
	return make_port(stdio_read, stdio_write, stream);
}

sexp_t port_peek_char(struct sexp_port *port)
{
	if (!port->buffer_full) {
		port->buffer = port->read_char(port);
		port->buffer_full = true;
	}
	return port->buffer;
}

sexp_t port_read_char(struct sexp_port *port)
{
	if (port->buffer_full) {
		port->buffer_full = false;
		return port->buffer;
	}
	return port->read_char(port);
}

void port_write_char(sexp_t ch, struct sexp_port *port)
{
	port->write_char(ch, port);
}

DEFUN(scm_current_input_port, args)
{
	return env_lookup(____env, sym_current_input);
}

DEFUN(scm_current_output_port, args)
{
	return env_lookup(____env, sym_current_output);
}

DEFUN(scm_current_error_port, args)
{
	return env_lookup(____env, sym_current_error);
}

#define get_port(fallback, args) _get_port(fallback, args, ____env)
static struct sexp_port *_get_port(builtin_t fallback, sexp_t args, env_t env)
{
	if (is_nil(args))
		return _port_cast(fallback(make_nil(), env), env);
	return _port_cast(car(args), env);
}

DEFUN(scm_read_u8, args)
{
	return port_read_char(get_port(scm_current_input_port, args));
}

DEFUN(scm_peek_u8, args)
{
	return port_peek_char(get_port(scm_current_input_port, args));
}

DEFUN(scm_read_char, args)
{
	return CALL(scm_read_u8, args); // TODO: decode UTF-8
}

DEFUN(scm_peek_char, args)
{
	return CALL(scm_peek_u8, args); // TODO: decode UTF-8
}

DEFUN(scm_write_u8, args)
{
	port_write_char(type_check(car(args), SEXP_CHAR),
			get_port(scm_current_output_port, cdr(args)));
	return unspecified();
}

DEFUN(scm_write_char, args)
{
	return CALL(scm_write_u8, args); // TODO: encode UTF-8
}
