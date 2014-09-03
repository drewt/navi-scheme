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

#define file_error(env, msg, ...) \
	error(env, msg, sym_file_error, ##__VA_ARGS__, make_void())

enum {
	GOT_EOF       = 1,
	BUFFER_FULL   = 2,
	INPUT_CLOSED  = 4,
	OUTPUT_CLOSED = 8,
};

static inline void check_input_port(struct sexp_port *p, env_t env)
{
	if (!p->read_u8)
		error(env, "not an input port");
}

static inline void check_output_port(struct sexp_port *p, env_t env)
{
	if (!p->write_u8)
		error(env, "not an output port");
}

static inline void check_can_read(struct sexp_port *p, env_t env)
{
	check_input_port(p, env);
	if (p->flags & INPUT_CLOSED)
		error(env, "attempted to read from close port");
}

void void_close(struct sexp_port *port, env_t env) {}

sexp_t stdio_read(struct sexp_port *port, env_t env)
{
	int c = getc(port->specific);
	return c == EOF ? make_eof() : make_char(c);
}

void stdio_write(sexp_t ch, struct sexp_port *port, env_t env)
{
	putc(sexp_char(ch), port->specific);
}

void stdio_close(struct sexp_port *port, env_t env)
{
	if (fclose(port->specific))
		error(env, "failed to close port");
}

static sexp_t string_read(struct sexp_port *port, env_t env)
{
	char c = sexp_string(port->sexp)->data[port->pos++];
	return c == '\0' ? make_eof() : make_char(c);
}

sexp_t make_string_input_port(sexp_t string)
{
	sexp_t port = make_port(string_read, NULL, NULL, NULL, NULL);
	port.p->data->port.sexp = string;
	return port;
}

sexp_t port_peek_char(struct sexp_port *port, env_t env)
{
	check_can_read(port, env);
	if (port->flags & GOT_EOF)
		return make_eof();
	if (!(port->flags & BUFFER_FULL)) {
		port->buffer = port->read_u8(port, env);
		port->flags |= BUFFER_FULL;
	}
	return port->buffer;
}

static sexp_t check_return(struct sexp_port *port, sexp_t sexp)
{
	if (is_eof(sexp))
		port->flags |= GOT_EOF;
	return sexp;
}

sexp_t port_read_char(struct sexp_port *port, env_t env)
{
	check_can_read(port, env);
	if (port->flags & GOT_EOF)
		return make_eof();
	if (port->flags & BUFFER_FULL) {
		port->flags &= ~BUFFER_FULL;
		return check_return(port, port->buffer);
	}
	return check_return(port, port->read_u8(port, env));
}

void port_write_char(sexp_t ch, struct sexp_port *port, env_t env)
{
	check_output_port(port, env);
	if (port->flags & OUTPUT_CLOSED)
		error(env, "attempted to write to closed port");
	port->write_u8(ch, port, env);
}

void port_write_c_string(const char *str, struct sexp_port *port, env_t env)
{
	while (*str != '\0')
		port_write_char(make_char(*str++), port, env);
}

static struct sexp_port *get_port(builtin_t fallback, sexp_t args, env_t env)
{
	if (is_nil(args))
		return port_cast(fallback(make_nil(), env), env);
	return port_cast(car(args), env);
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

DEFUN(scm_open_input_file, args, env)
{
	FILE *f;

	type_check(car(args), SEXP_STRING, env);

	if ((f = fopen(sexp_string(car(args))->data, "r")) == NULL) {
		file_error(env, "unable to open file");
	}

	return make_input_port(stdio_read, stdio_close, f);
}

DEFUN(scm_open_output_file, args, env)
{
	FILE *f;

	type_check(car(args), SEXP_STRING, env);

	if ((f = fopen(sexp_string(car(args))->data, "w")) == NULL) {
		file_error(env, "unable to open file");
	}

	return make_output_port(stdio_write, stdio_close, f);
}

DEFUN(scm_open_input_string, args, env)
{
	return make_string_input_port(type_check(car(args), SEXP_STRING, env));
}

static void close_input_port(struct sexp_port *p, env_t env)
{
	p->close_in(p, env);
	p->close_in = void_close;
	p->flags |= INPUT_CLOSED;
}

static void close_output_port(struct sexp_port *p, env_t env)
{
	p->close_out(p, env);
	p->close_out = void_close;
	p->flags |= OUTPUT_CLOSED;
}

DEFUN(scm_close_port, args, env)
{
	struct sexp_port *p = port_cast(car(args), env);

	if (p->read_u8)
		close_input_port(p, env);
	if (p->write_u8)
		close_output_port(p, env);

	return unspecified();
}

DEFUN(scm_close_input_port, args, env)
{
	struct sexp_port *p = port_cast(car(args), env);
	check_input_port(p, env);
	close_input_port(p, env);
	return unspecified();
}

DEFUN(scm_close_output_port, args, env)
{
	struct sexp_port *p = port_cast(car(args), env);
	check_output_port(p, env);
	close_output_port(p, env);
	return unspecified();
}

DEFUN(scm_eof_objectp, args, env)
{
	return make_bool(is_eof(car(args)));
}

DEFUN(scm_eof_object, args, env)
{
	return make_eof();
}

DEFUN(scm_read_u8, args, env)
{
	return port_read_char(get_port(scm_current_input_port, args, env), env);
}

DEFUN(scm_peek_u8, args, env)
{
	struct sexp_port *p = get_port(scm_current_input_port, args, env);
	check_input_port(p, env);
	return port_peek_char(p, env);
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
	struct sexp_port *p = get_port(scm_current_input_port, args, env);
	check_input_port(p, env);
	return sexp_read(p, env);
}

DEFUN(scm_write_u8, args, env)
{
	struct sexp_port *p = get_port(scm_current_output_port, cdr(args), env);
	check_output_port(p, env);
	port_write_char(type_check(car(args), SEXP_CHAR, env), p, env);
	return unspecified();
}

DEFUN(scm_write_char, args, env)
{
	return scm_write_u8(args, env); // TODO: encode UTF-8
}

DEFUN(scm_write_string, args, env)
{
	struct sexp_port *p = get_port(scm_current_output_port, cdr(args), env);
	check_output_port(p, env);
	type_check(car(args), SEXP_STRING, env);
	port_write_c_string(sexp_string(car(args))->data, p, env);
	return unspecified();
}

DEFUN(scm_display, args, env)
{
	struct sexp_port *p = get_port(scm_current_output_port, cdr(args), env);
	check_output_port(p, env);
	_display(p, car(args), false, env);
	return unspecified();
}

DEFUN(scm_write, args, env)
{
	struct sexp_port *p = get_port(scm_current_output_port, cdr(args), env);
	check_output_port(p, env);
	_display(p, car(args), true, env);
	return unspecified();
}

DEFUN(scm_newline, args, env)
{
	scm_write_u8(make_pair(make_char('\n'), args), env);
	return unspecified();
}
