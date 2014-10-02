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

#include "navi.h"

#define STRING_INIT_SIZE 64
#define STRING_STEP_SIZE 64

#define file_error(env, msg, ...) \
	error(env, msg, sym_file_error, ##__VA_ARGS__, make_void())

enum {
	GOT_EOF       = 1,
	BUFFER_FULL   = 2,
	INPUT_CLOSED  = 4,
	OUTPUT_CLOSED = 8,
	STRING_OUTPUT = 16,
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

int stdio_read(struct sexp_port *port, env_t env)
{
	return getc(port->specific);
}

void stdio_write(unsigned char ch, struct sexp_port *port, env_t env)
{
	putc(ch, port->specific);
}

void stdio_close(struct sexp_port *port, env_t env)
{
	if (fclose(port->specific))
		error(env, "failed to close port");
}

static int string_read(struct sexp_port *port, env_t env)
{
	unsigned char c = sexp_string(port->sexp)->data[port->pos++];
	return c == '\0' ? EOF : c;
}

static void string_write(unsigned char ch, struct sexp_port *port, env_t env)
{
	struct sexp_string *str = sexp_string(port->sexp);

	if (str->size + 1 >= str->storage) {
		str->data = xrealloc(str->data, str->storage + STRING_STEP_SIZE);
		str->storage += STRING_STEP_SIZE;
	}

	str->data[str->size++] = ch;
	str->data[str->size] = '\0';
	str->length++;
}

sexp_t make_string_input_port(sexp_t string)
{
	sexp_t port = make_port(string_read, NULL, NULL, NULL, NULL);
	port.p->data->port.sexp = string;
	return port;
}

sexp_t make_string_output_port(void)
{
	sexp_t port = make_port(NULL, string_write, NULL, NULL, NULL);
	port.p->data->port.sexp = make_string(STRING_INIT_SIZE, 0, 0);
	port.p->data->port.flags |= STRING_OUTPUT;
	return port;
}

static void port_buffer_byte(struct sexp_port *port, env_t env)
{
	int byte = port->read_u8(port, env);
	port->buffer = (byte == EOF) ? UCHAR_EOF : byte;
}

static uchar port_peek_c_byte(struct sexp_port *port, env_t env)
{
	check_can_read(port, env);
	if (port->flags & GOT_EOF)
		return UCHAR_EOF;
	if (!(port->flags & BUFFER_FULL)) {
		port_buffer_byte(port, env);
		port->flags |= BUFFER_FULL;
	}
	return port->buffer;
}

sexp_t port_peek_byte(struct sexp_port *port, env_t env)
{
	uchar b = port_peek_c_byte(port, env);
	return b == UCHAR_EOF ? make_eof() : make_num(b);
}

static uchar port_read_c_byte(struct sexp_port *port, env_t env)
{
	check_can_read(port, env);
	if (port->flags & GOT_EOF)
		return UCHAR_EOF;
	if (port->flags & BUFFER_FULL)
		port->flags &= ~BUFFER_FULL;
	else
		port_buffer_byte(port, env);
	return port->buffer;
}

sexp_t port_read_byte(struct sexp_port *port, env_t env)
{
	uchar b = port_read_c_byte(port, env);
	if (b == UCHAR_EOF) {
		port->flags |= GOT_EOF;
		return make_eof();
	}
	return make_num(b);
}

static void port_buffer_char(struct sexp_port *port, env_t env)
{
	int size;
	int byte;
	char buffer[4];

	if ((byte = port->read_u8(port, env)) == EOF) {
		port->buffer = UCHAR_EOF;
		port->flags |= GOT_EOF;
		return;
	}
	buffer[0] = byte;

	size = utf8_char_size(buffer[0]);
	for (int i = 1; i < size; i++) {
		if ((byte = port->read_u8(port, env)) == EOF)
			error(env, "unexpected end of file");
		buffer[i] = byte;
	}

	port->buffer = u_get_char(buffer, NULL);
}

sexp_t port_read_char(struct sexp_port *port, env_t env)
{
	check_can_read(port, env);
	if (port->flags & GOT_EOF)
		return make_eof();
	if (port->flags & BUFFER_FULL) {
		port->flags &= ~BUFFER_FULL;
		return make_char(port->buffer);
	}
	port_buffer_char(port, env);
	return make_char(port->buffer);
}

sexp_t port_peek_char(struct sexp_port *port, env_t env)
{
	check_can_read(port, env);
	if (port->flags & GOT_EOF)
		return make_eof();
	if (!(port->flags & BUFFER_FULL)) {
		port_buffer_char(port, env);
		port->flags |= BUFFER_FULL;
	}
	return make_char(port->buffer);
}

void port_write_byte(unsigned char ch, struct sexp_port *port, env_t env)
{
	check_output_port(port, env);
	if (port->flags & OUTPUT_CLOSED)
		error(env, "attempted to write to closed port");
	port->write_u8(ch, port, env);
}

void port_write_char(uchar ch, struct sexp_port *port, env_t env)
{
	char buf[5];
	int size;
	size_t zero = 0;

	check_output_port(port, env);
	if (port->flags & OUTPUT_CLOSED)
		error(env, "attempted to write to closed port");

	u_set_char_raw(buf, &zero, ch);
	size = utf8_char_size(buf[0]);
	for (int i = 0; i < size; i++)
		port->write_u8(buf[i], port, env);
}

void port_write_c_string(const char *str, struct sexp_port *port, env_t env)
{
	while (*str != '\0')
		port_write_byte(*str++, port, env);
}

static struct sexp_port *get_port(builtin_t fallback, sexp_t args, env_t env)
{
	if (is_nil(args))
		return port_cast(fallback(make_nil(), env), env);
	return port_cast(car(args), env);
}

static inline struct sexp_port *get_input_port(sexp_t args, env_t env)
{
	struct sexp_port *p = get_port(scm_current_input_port, args, env);
	check_input_port(p, env);
	return p;
}

static inline struct sexp_port *get_output_port(sexp_t args, env_t env)
{
	struct sexp_port *p = get_port(scm_current_output_port, args, env);
	check_output_port(p, env);
	return p;
}

DEFUN(scm_input_portp, args, env)
{
	return make_bool(sexp_type(car(args)) == SEXP_PORT
			&& sexp_port(car(args))->read_u8);
}

DEFUN(scm_output_portp, args, env)
{
	return make_bool(sexp_type(car(args)) == SEXP_PORT
			&& sexp_port(car(args))->write_u8);
}

DEFUN(scm_portp, args, env)
{
	return make_bool(sexp_type(car(args)) == SEXP_PORT);
}

DEFUN(scm_input_port_openp, args, env)
{
	struct sexp_port *p = port_cast(car(args), env);
	check_input_port(p, env);
	return make_bool(!(p->flags & INPUT_CLOSED));
}

DEFUN(scm_output_port_openp, args, env)
{
	struct sexp_port *p = port_cast(car(args), env);
	check_output_port(p, env);
	return make_bool(!(p->flags & OUTPUT_CLOSED));
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

DEFUN(scm_open_output_string, args, env)
{
	return make_string_output_port();
}

DEFUN(scm_get_output_string, args, env)
{
	sexp_t sexp;
	struct sexp_string *str;
	struct sexp_port *p = port_cast(car(args), env);

	if (!(p->flags & STRING_OUTPUT))
		error(env, "not a string output port");

	str = sexp_string(p->sexp);
	sexp = make_string(str->size, str->size, str->length);
	memcpy(sexp_string(sexp)->data, str->data, str->size);
	return sexp;
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
	struct sexp_port *p = get_input_port(args, env);
	return port_read_byte(p, env);
}

DEFUN(scm_peek_u8, args, env)
{
	struct sexp_port *p = get_input_port(args, env);
	return port_peek_byte(p, env);
}

DEFUN(scm_read_char, args, env)
{
	struct sexp_port *p = get_input_port(args, env);
	return port_read_char(p, env);
}

DEFUN(scm_peek_char, args, env)
{
	struct sexp_port *p = get_input_port(args, env);
	return port_peek_char(p, env);
}

DEFUN(scm_read, args, env)
{
	struct sexp_port *p = get_input_port(args, env);
	return sexp_read(p, env);
}

DEFUN(scm_write_u8, args, env)
{
	struct sexp_port *p = get_output_port(cdr(args), env);
	port_write_char(type_check_byte(car(args), env), p, env);
	return unspecified();
}

DEFUN(scm_write_char, args, env)
{
	struct sexp_port *p = get_output_port(cdr(args), env);
	port_write_char(char_cast(car(args), env), p, env);
	return unspecified();
}

DEFUN(scm_write_string, args, env)
{
	struct sexp_port *p = get_output_port(cdr(args), env);
	type_check(car(args), SEXP_STRING, env);
	port_write_c_string(sexp_string(car(args))->data, p, env);
	return unspecified();
}

DEFUN(scm_display, args, env)
{
	struct sexp_port *p = get_output_port(cdr(args), env);
	_display(p, car(args), false, env);
	return unspecified();
}

DEFUN(scm_write, args, env)
{
	struct sexp_port *p = get_output_port(cdr(args), env);
	_display(p, car(args), true, env);
	return unspecified();
}

DEFUN(scm_newline, args, env)
{
	scm_write_u8(make_pair(make_char('\n'), args), env);
	return unspecified();
}
