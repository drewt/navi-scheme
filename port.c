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

#include "navi.h"
#include "navi/uchar.h"

#define STRING_INIT_SIZE 64
#define STRING_STEP_SIZE 64

enum {
	GOT_EOF       = 1,
	BUFFER_FULL   = 2,
	INPUT_CLOSED  = 4,
	OUTPUT_CLOSED = 8,
	STRING_OUTPUT = 16,
};

static inline void check_input_port(struct navi_port *p, navi_env_t env)
{
	if (!p->read_u8)
		navi_error(env, "not an input port");
}

static inline void check_output_port(struct navi_port *p, navi_env_t env)
{
	if (!p->write_u8)
		navi_error(env, "not an output port");
}

static inline void check_can_read(struct navi_port *p, navi_env_t env)
{
	check_input_port(p, env);
	if (p->flags & INPUT_CLOSED)
		navi_error(env, "attempted to read from close port");
}

static void void_close(struct navi_port *port, navi_env_t env) {}

static int stdio_read(struct navi_port *port, navi_env_t env)
{
	return getc(port->specific);
}

static void stdio_write(unsigned char ch, struct navi_port *port, navi_env_t env)
{
	putc(ch, port->specific);
}

static void stdio_close(struct navi_port *port, navi_env_t env)
{
	if (fclose(port->specific))
		navi_error(env, "failed to close port");
}

navi_t navi_make_file_input_port(FILE *file)
{
	return navi_make_input_port(stdio_read, stdio_close, file);
}

navi_t navi_make_file_output_port(FILE *file)
{
	return navi_make_output_port(stdio_write, stdio_close, file);
}

static int string_read(struct navi_port *port, navi_env_t env)
{
	unsigned char c = navi_string(port->expr)->data[port->pos++];
	return c == '\0' ? EOF : c;
}

static void string_write(unsigned char ch, struct navi_port *port, navi_env_t env)
{
	struct navi_string *str = navi_string(port->expr);

	if (str->size + 1 >= str->storage) {
		str->data = navi_critical_realloc(str->data, str->storage + STRING_STEP_SIZE);
		str->storage += STRING_STEP_SIZE;
	}

	str->data[str->size++] = ch;
	str->data[str->size] = '\0';
	str->length++;
}

static navi_t make_string_input_port(navi_t string)
{
	navi_t port = navi_make_port(string_read, NULL, NULL, NULL, NULL);
	port.p->data->port.expr = string;
	return port;
}

static navi_t make_string_output_port(void)
{
	navi_t port = navi_make_port(NULL, string_write, NULL, NULL, NULL);
	port.p->data->port.expr = navi_make_string(STRING_INIT_SIZE, 0, 0);
	port.p->data->port.flags |= STRING_OUTPUT;
	return port;
}

static void navi_port_buffer_byte(struct navi_port *port, navi_env_t env)
{
	int byte = port->read_u8(port, env);
	port->buffer = (byte == EOF) ? UCHAR_EOF : byte;
}

static uchar navi_port_peek_c_byte(struct navi_port *port, navi_env_t env)
{
	check_can_read(port, env);
	if (port->flags & GOT_EOF)
		return UCHAR_EOF;
	if (!(port->flags & BUFFER_FULL)) {
		navi_port_buffer_byte(port, env);
		port->flags |= BUFFER_FULL;
	}
	return port->buffer;
}

navi_t navi_port_peek_byte(struct navi_port *port, navi_env_t env)
{
	uchar b = navi_port_peek_c_byte(port, env);
	return b == UCHAR_EOF ? navi_make_eof() : navi_make_num(b);
}

static uchar navi_port_read_c_byte(struct navi_port *port, navi_env_t env)
{
	check_can_read(port, env);
	if (port->flags & GOT_EOF)
		return UCHAR_EOF;
	if (port->flags & BUFFER_FULL)
		port->flags &= ~BUFFER_FULL;
	else
		navi_port_buffer_byte(port, env);
	return port->buffer;
}

navi_t navi_port_read_byte(struct navi_port *port, navi_env_t env)
{
	uchar b = navi_port_read_c_byte(port, env);
	if (b == UCHAR_EOF) {
		port->flags |= GOT_EOF;
		return navi_make_eof();
	}
	return navi_make_num(b);
}

static void navi_port_buffer_char(struct navi_port *port, navi_env_t env)
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
			navi_error(env, "unexpected end of file");
		buffer[i] = byte;
	}

	port->buffer = u_get_char(buffer, NULL);
}

navi_t navi_port_read_char(struct navi_port *port, navi_env_t env)
{
	check_can_read(port, env);
	if (port->flags & GOT_EOF)
		return navi_make_eof();
	if (port->flags & BUFFER_FULL) {
		port->flags &= ~BUFFER_FULL;
		return navi_make_char(port->buffer);
	}
	navi_port_buffer_char(port, env);
	return navi_make_char(port->buffer);
}

navi_t navi_port_peek_char(struct navi_port *port, navi_env_t env)
{
	check_can_read(port, env);
	if (port->flags & GOT_EOF)
		return navi_make_eof();
	if (!(port->flags & BUFFER_FULL)) {
		navi_port_buffer_char(port, env);
		port->flags |= BUFFER_FULL;
	}
	return navi_make_char(port->buffer);
}

void navi_port_write_byte(unsigned char ch, struct navi_port *port, navi_env_t env)
{
	check_output_port(port, env);
	if (port->flags & OUTPUT_CLOSED)
		navi_error(env, "attempted to write to closed port");
	port->write_u8(ch, port, env);
}

void navi_port_write_char(uchar ch, struct navi_port *port, navi_env_t env)
{
	char buf[5];
	int size;
	size_t zero = 0;

	check_output_port(port, env);
	if (port->flags & OUTPUT_CLOSED)
		navi_error(env, "attempted to write to closed port");

	u_set_char_raw(buf, &zero, ch);
	size = utf8_char_size(buf[0]);
	for (int i = 0; i < size; i++)
		port->write_u8(buf[i], port, env);
}

void navi_port_write_cstr(const char *str, struct navi_port *port, navi_env_t env)
{
	while (*str != '\0')
		navi_port_write_byte(*str++, port, env);
}

static struct navi_port *get_port(navi_builtin_t fallback, navi_t args, navi_env_t env)
{
	if (navi_is_nil(args))
		return navi_port_cast(fallback(navi_make_nil(), env), env);
	return navi_port_cast(navi_car(args), env);
}

static inline struct navi_port *get_input_port(navi_t args, navi_env_t env)
{
	struct navi_port *p = get_port(scm_current_input_port, args, env);
	check_input_port(p, env);
	return p;
}

static inline struct navi_port *get_output_port(navi_t args, navi_env_t env)
{
	struct navi_port *p = get_port(scm_current_output_port, args, env);
	check_output_port(p, env);
	return p;
}

DEFUN(scm_input_portp, args, env)
{
	return navi_make_bool(navi_type(navi_car(args)) == NAVI_PORT
			&& navi_port(navi_car(args))->read_u8);
}

DEFUN(scm_output_portp, args, env)
{
	return navi_make_bool(navi_type(navi_car(args)) == NAVI_PORT
			&& navi_port(navi_car(args))->write_u8);
}

DEFUN(scm_portp, args, env)
{
	return navi_make_bool(navi_type(navi_car(args)) == NAVI_PORT);
}

DEFUN(scm_input_port_openp, args, env)
{
	struct navi_port *p = navi_port_cast(navi_car(args), env);
	check_input_port(p, env);
	return navi_make_bool(!(p->flags & INPUT_CLOSED));
}

DEFUN(scm_output_port_openp, args, env)
{
	struct navi_port *p = navi_port_cast(navi_car(args), env);
	check_output_port(p, env);
	return navi_make_bool(!(p->flags & OUTPUT_CLOSED));
}

DEFUN(scm_current_input_port, args, env)
{
	return navi_env_lookup(env, navi_sym_current_input);
}

DEFUN(scm_current_output_port, args, env)
{
	return navi_env_lookup(env, navi_sym_current_output);
}

DEFUN(scm_current_error_port, args, env)
{
	return navi_env_lookup(env, navi_sym_current_error);
}

DEFUN(scm_open_input_file, args, env)
{
	FILE *f;

	navi_type_check(navi_car(args), NAVI_STRING, env);

	if ((f = fopen(navi_string(navi_car(args))->data, "r")) == NULL) {
		navi_file_error(env, "unable to open file");
	}

	return navi_make_input_port(stdio_read, stdio_close, f);
}

DEFUN(scm_open_output_file, args, env)
{
	FILE *f;

	navi_type_check(navi_car(args), NAVI_STRING, env);

	if ((f = fopen(navi_string(navi_car(args))->data, "w")) == NULL) {
		navi_file_error(env, "unable to open file");
	}

	return navi_make_output_port(stdio_write, stdio_close, f);
}

DEFUN(scm_open_input_string, args, env)
{
	return make_string_input_port(navi_type_check(navi_car(args), NAVI_STRING, env));
}

DEFUN(scm_open_output_string, args, env)
{
	return make_string_output_port();
}

DEFUN(scm_get_output_string, args, env)
{
	navi_t expr;
	struct navi_string *str;
	struct navi_port *p = navi_port_cast(navi_car(args), env);

	if (!(p->flags & STRING_OUTPUT))
		navi_error(env, "not a string output port");

	str = navi_string(p->expr);
	expr = navi_make_string(str->size, str->size, str->length);
	memcpy(navi_string(expr)->data, str->data, str->size);
	return expr;
}

static void close_input_port(struct navi_port *p, navi_env_t env)
{
	p->close_in(p, env);
	p->close_in = void_close;
	p->flags |= INPUT_CLOSED;
}

static void close_output_port(struct navi_port *p, navi_env_t env)
{
	p->close_out(p, env);
	p->close_out = void_close;
	p->flags |= OUTPUT_CLOSED;
}

DEFUN(scm_close_port, args, env)
{
	struct navi_port *p = navi_port_cast(navi_car(args), env);

	if (p->read_u8)
		close_input_port(p, env);
	if (p->write_u8)
		close_output_port(p, env);

	return navi_unspecified();
}

DEFUN(scm_close_input_port, args, env)
{
	struct navi_port *p = navi_port_cast(navi_car(args), env);
	check_input_port(p, env);
	close_input_port(p, env);
	return navi_unspecified();
}

DEFUN(scm_close_output_port, args, env)
{
	struct navi_port *p = navi_port_cast(navi_car(args), env);
	check_output_port(p, env);
	close_output_port(p, env);
	return navi_unspecified();
}

DEFUN(scm_eof_objectp, args, env)
{
	return navi_make_bool(navi_is_eof(navi_car(args)));
}

DEFUN(scm_eof_object, args, env)
{
	return navi_make_eof();
}

DEFUN(scm_read_u8, args, env)
{
	struct navi_port *p = get_input_port(args, env);
	return navi_port_read_byte(p, env);
}

DEFUN(scm_peek_u8, args, env)
{
	struct navi_port *p = get_input_port(args, env);
	return navi_port_peek_byte(p, env);
}

DEFUN(scm_read_char, args, env)
{
	struct navi_port *p = get_input_port(args, env);
	return navi_port_read_char(p, env);
}

DEFUN(scm_peek_char, args, env)
{
	struct navi_port *p = get_input_port(args, env);
	return navi_port_peek_char(p, env);
}

DEFUN(scm_read, args, env)
{
	struct navi_port *p = get_input_port(args, env);
	return navi_read(p, env);
}

DEFUN(scm_write_u8, args, env)
{
	struct navi_port *p = get_output_port(navi_cdr(args), env);
	navi_port_write_char(navi_type_check_byte(navi_car(args), env), p, env);
	return navi_unspecified();
}

DEFUN(scm_write_char, args, env)
{
	struct navi_port *p = get_output_port(navi_cdr(args), env);
	navi_port_write_char(navi_char_cast(navi_car(args), env), p, env);
	return navi_unspecified();
}

DEFUN(scm_write_string, args, env)
{
	struct navi_port *p = get_output_port(navi_cdr(args), env);
	navi_type_check(navi_car(args), NAVI_STRING, env);
	navi_port_write_cstr(navi_string(navi_car(args))->data, p, env);
	return navi_unspecified();
}

DEFUN(scm_display, args, env)
{
	struct navi_port *p = get_output_port(navi_cdr(args), env);
	_navi_display(p, navi_car(args), false, env);
	return navi_unspecified();
}

DEFUN(scm_write, args, env)
{
	struct navi_port *p = get_output_port(navi_cdr(args), env);
	_navi_display(p, navi_car(args), true, env);
	return navi_unspecified();
}

DEFUN(scm_newline, args, env)
{
	scm_write_u8(navi_make_pair(navi_make_char('\n'), args), env);
	return navi_unspecified();
}
