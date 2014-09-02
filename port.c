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
	int c = getc(port->specific);
	return c == EOF ? make_eof() : make_char(c);
}

static void stdio_write(sexp_t ch, struct sexp_port *port)
{
	putc(sexp_char(ch), port->specific);
}

sexp_t make_stdio_port(FILE *stream)
{
	return make_port(stdio_read, stdio_write, stream);
}

static sexp_t string_read(struct sexp_port *port)
{
	char c = sexp_string(port->sexp)->data[port->pos++];
	return c == '\0' ? make_eof() : make_char(c);
}

sexp_t make_string_input_port(sexp_t string)
{
	sexp_t port = make_port(string_read, NULL, NULL);
	port.p->data->port.sexp = string;
	return port;
}

sexp_t port_peek_char(struct sexp_port *port)
{
	if (port->eof)
		return make_eof();
	if (!port->buffer_full) {
		port->buffer = port->read_u8(port);
		port->buffer_full = true;
	}
	return port->buffer;
}

static sexp_t check_return(struct sexp_port *port, sexp_t sexp)
{
	if (is_eof(sexp))
		port->eof = true;
	return sexp;
}

sexp_t port_read_char(struct sexp_port *port)
{
	if (port->eof)
		return make_eof();
	if (port->buffer_full) {
		port->buffer_full = false;
		return check_return(port, port->buffer);
	}
	return check_return(port, port->read_u8(port));
}

void port_write_char(sexp_t ch, struct sexp_port *port)
{
	port->write_u8(ch, port);
}

void port_write_c_string(const char *str, struct sexp_port *port)
{
	while (*str != '\0')
		port_write_char(make_char(*str++), port);
}

DEFUN(scm_current_input_port, args, env)
{
	return env_lookup(env, sym_current_input);
}

DEFUN(scm_current_output_port, args, env)
{
	return env_lookup(env, sym_current_output);
}

DEFUN(scm_current_error_port, args, env)
{
	return env_lookup(env, sym_current_error);
}

DEFUN(scm_open_input_string, args, env)
{
	return make_string_input_port(type_check(car(args), SEXP_STRING, env));
}

DEFUN(scm_eof_objectp, args, env)
{
	return make_bool(is_eof(car(args)));
}

DEFUN(scm_eof_object, args, env)
{
	return make_eof();
}

static struct sexp_port *get_port(builtin_t fallback, sexp_t args, env_t env)
{
	if (is_nil(args))
		return port_cast(fallback(make_nil(), env), env);
	return port_cast(car(args), env);
}

DEFUN(scm_read_u8, args, env)
{
	return port_read_char(get_port(scm_current_input_port, args, env));
}

DEFUN(scm_peek_u8, args, env)
{
	return port_peek_char(get_port(scm_current_input_port, args, env));
}

DEFUN(scm_read_char, args, env)
{
	return scm_read_u8(args, env); // TODO: decode UTF-8
}

DEFUN(scm_peek_char, args, env)
{
	return scm_peek_u8(args, env); // TODO: decode UTF-8
}

DEFUN(scm_read, args, env)
{
	return sexp_read(get_port(scm_current_input_port, args, env), env);
}

DEFUN(scm_write_u8, args, env)
{
	port_write_char(type_check(car(args), SEXP_CHAR, env),
			get_port(scm_current_output_port, cdr(args), env));
	return unspecified();
}

DEFUN(scm_write_char, args, env)
{
	return scm_write_u8(args, env); // TODO: encode UTF-8
}

DEFUN(scm_write_string, args, env)
{
	type_check(car(args), SEXP_STRING, env);
	port_write_c_string(sexp_string(car(args))->data,
			get_port(scm_current_output_port, cdr(args), env));
	return unspecified();
}

DEFUN(scm_display, args, env)
{
	_display(get_port(scm_current_output_port, cdr(args), env), car(args),
			false);
	return unspecified();
}

DEFUN(scm_write, args, env)
{
	_display(get_port(scm_current_output_port, cdr(args), env), car(args),
			true);
	return unspecified();
}

DEFUN(scm_newline, args, env)
{
	scm_write_u8(make_pair(make_char('\n'), args), env);
	return unspecified();
}
