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
#include "navi/unicode.h"

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
	if (!p->read_u8 && !p->read_char)
		navi_error(env, "not an input port");
}

static inline void check_output_port(struct navi_port *p, navi_env_t env)
{
	if (!p->write_u8 && !p->write_char)
		navi_error(env, "not an output port");
}

static inline void check_can_read(struct navi_port *p, navi_env_t env)
{
	check_input_port(p, env);
	if (p->flags & INPUT_CLOSED)
		navi_error(env, "attempted to read from closed port");
}

static inline void check_can_read_u8(struct navi_port *p, navi_env_t env)
{
	check_can_read(p, env);
	if (!p->read_u8)
		navi_error(env, "port doesn't support binary input");
}

static inline void check_can_write(struct navi_port *p, navi_env_t env)
{
	check_output_port(p, env);
	if (p->flags & OUTPUT_CLOSED)
		navi_error(env, "attempted to write to closed port");
}

static inline void check_can_write_u8(struct navi_port *p, navi_env_t env)
{
	check_can_write(p, env);
	if (!p->write_u8)
		navi_error(env, "port doesn't support binary output");
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
	return navi_make_input_port(stdio_read, NULL, stdio_close, file);
}

navi_t navi_make_file_output_port(FILE *file)
{
	return navi_make_output_port(stdio_write, NULL, stdio_close, file);
}

static int32_t string_read(struct navi_port *port, navi_env_t env)
{
	UChar32 ch;
	struct navi_string *str = navi_string(port->expr);
	if (port->pos == str->size)
		return EOF;
	u8_next(str->data, port->pos, str->size, ch);
	return ch;
}

static void string_write(int32_t ch, struct navi_port *port, navi_env_t env)
{
	struct navi_string *str = navi_string(port->expr);

	navi_string_grow_storage(str, u8_length(ch));
	u8_append(str->data, str->size, str->capacity, ch);
	str->data[str->size] = '\0';
	str->length++;
}

static navi_t make_string_input_port(navi_t string)
{
	navi_t port = navi_make_textual_input_port(string_read, NULL, NULL);
	navi_port(port)->expr = string;
	return port;
}

static navi_t make_string_output_port(void)
{
	navi_t port = navi_make_textual_output_port(string_write, NULL, NULL);
	port.p->data->port.expr = navi_make_string(STRING_INIT_SIZE, 0, 0);
	port.p->data->port.flags |= STRING_OUTPUT;
	return port;
}

static void navi_port_buffer_byte(struct navi_port *port, navi_env_t env)
{
	port->buffer = port->read_u8(port, env);
}

static int32_t navi_port_peek_c_byte(struct navi_port *port, navi_env_t env)
{
	check_can_read_u8(port, env);
	if (port->flags & GOT_EOF)
		return EOF;
	if (!(port->flags & BUFFER_FULL)) {
		navi_port_buffer_byte(port, env);
		port->flags |= BUFFER_FULL;
	}
	return port->buffer;
}

navi_t navi_port_peek_byte(struct navi_port *port, navi_env_t env)
{
	int32_t b = navi_port_peek_c_byte(port, env);
	return b == EOF ? navi_make_eof() : navi_make_num(b);
}

static int32_t navi_port_read_c_byte(struct navi_port *port, navi_env_t env)
{
	check_can_read_u8(port, env);
	if (port->flags & GOT_EOF)
		return EOF;
	if (port->flags & BUFFER_FULL)
		port->flags &= ~BUFFER_FULL;
	else
		navi_port_buffer_byte(port, env);
	return port->buffer;
}

navi_t navi_port_read_byte(struct navi_port *port, navi_env_t env)
{
	int32_t b = navi_port_read_c_byte(port, env);
	if (b == EOF) {
		port->flags |= GOT_EOF;
		return navi_make_eof();
	}
	return navi_make_num(b);
}

static const signed char len_tab[256] = {
	/*   0-127  0xxxxxxx */
	1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1,
	1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1,
	1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1,
	1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1,
	1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1,
	1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1,
	1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1,
	1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1,

	/* 128-191  10xxxxxx (invalid first byte) */
	0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
	0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
	0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
	0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,

	/* 192-223  110xxxxx */
	2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2,
	2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2,

	/* 224-239  1110xxxx */
	3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3,

	/* 240-244  11110xxx (000 - 100) */
	4, 4, 4, 4, 4,

	/* 11110xxx (101 - 111) (always invalid) */
	-1, -1, -1,

	/* 11111xxx (always invalid) */
	-1, -1, -1, -1, -1, -1, -1, -1
};

static inline int utf8_char_size(unsigned char ch)
{
	return len_tab[ch];
}

#define navi_utf8_error(env, msg, ...) \
	navi_error(env, msg, navi_make_symbol("#utf8-error"), \
			##__VA_ARGS__)

static void navi_port_buffer_char(struct navi_port *port, navi_env_t env)
{
	int size;
	int byte;
	uint8_t buffer[4];

	if (port->read_char) {
		port->buffer = port->read_char(port, env);
		if (port->buffer == EOF)
			port->flags |= GOT_EOF;
		return;
	}

	// fallback: use read_u8 and decode UTF-8
	if ((byte = port->read_u8(port, env)) == EOF) {
		port->buffer = EOF;
		port->flags |= GOT_EOF;
		return;
	}
	buffer[0] = byte;

	size = utf8_char_size(buffer[0]);
	if (size <= 0)
		navi_utf8_error(env, "invalid lead byte");
	for (int i = 1; i < size; i++) {
		if ((byte = port->read_u8(port, env)) == EOF)
			navi_utf8_error(env, "unexpected end of file");
		buffer[i] = byte;
	}
	u8_get_unchecked(buffer, 0, 0, 4, port->buffer);
	if (port->buffer < 0)
		navi_utf8_error(env, "invalid byte sequence");
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
	if (port->flags & GOT_EOF)
		return navi_make_eof();
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
	check_can_write_u8(port, env);
	port->write_u8(ch, port, env);
}

void navi_port_write_char(int32_t ch, struct navi_port *port, navi_env_t env)
{
	uint8_t buf[5];
	int size;
	int32_t zero = 0;

	check_can_write(port, env);
	if (port->write_char) {
		port->write_char(ch, port, env);
		return;
	}

	// fallback: encode UTF-8 and use write_u8
	u8_append(buf, zero, 5, ch);
	size = u8_length(ch);
	for (int i = 0; i < size; i++)
		port->write_u8(buf[i], port, env);
}

void navi_port_write_cstr(const char *str, struct navi_port *port, navi_env_t env)
{
	while (*str != '\0')
		navi_port_write_byte(*str++, port, env);
}

navi_t navi_current_input_port(navi_env_t env)
{
	return navi_env_lookup(env, navi_sym_current_input);
}

navi_t navi_current_output_port(navi_env_t env)
{
	return navi_env_lookup(env, navi_sym_current_output);
}

navi_t navi_current_error_port(navi_env_t env)
{
	return navi_env_lookup(env, navi_sym_current_error);
}

static struct navi_port *get_port(navi_t fallback, navi_t args, navi_env_t env)
{
	if (navi_is_nil(args))
		return navi_port_cast(navi_env_lookup(env, fallback), env);
	return navi_port_cast(navi_car(args), env);
}

static inline struct navi_port *get_input_port(navi_t args, navi_env_t env)
{
	struct navi_port *p = get_port(navi_sym_current_input, args, env);
	check_input_port(p, env);
	return p;
}

static inline struct navi_port *get_output_port(navi_t args, navi_env_t env)
{
	struct navi_port *p = get_port(navi_sym_current_output, args, env);
	check_output_port(p, env);
	return p;
}

DEFUN(input_portp, args, env, "input-port?", 1, 0, NAVI_ANY)
{
	return navi_make_bool(navi_type(navi_car(args)) == NAVI_PORT
			&& navi_port(navi_car(args))->read_u8);
}

DEFUN(output_portp, args, env, "output-port?", 1, 0, NAVI_ANY)
{
	return navi_make_bool(navi_type(navi_car(args)) == NAVI_PORT
			&& navi_port(navi_car(args))->write_u8);
}

DEFUN(portp, args, env, "port?", 1, 0, NAVI_ANY)
{
	return navi_make_bool(navi_type(navi_car(args)) == NAVI_PORT);
}

DEFUN(input_port_openp, args, env, "input-port-open?", 1, 0, NAVI_PORT)
{
	struct navi_port *p = navi_port_cast(navi_car(args), env);
	check_input_port(p, env);
	return navi_make_bool(!(p->flags & INPUT_CLOSED));
}

DEFUN(output_port_openp, args, env, "output-port-open?", 1, 0, NAVI_PORT)
{
	struct navi_port *p = navi_port_cast(navi_car(args), env);
	check_output_port(p, env);
	return navi_make_bool(!(p->flags & OUTPUT_CLOSED));
}

DEFUN(current_input_port, args, env, "current-input-port", 0, 0)
{
	return navi_current_input_port(env);
}

DEFUN(current_output_port, args, env, "current-output-port", 0, 0)
{
	return navi_current_output_port(env);
}

DEFUN(current_error_port, args, env, "current-error-port", 0, 0)
{
	return navi_current_error_port(env);
}

DEFUN(open_input_file, args, env, "open-input-file", 1, 0, NAVI_STRING)
{
	FILE *f;

	navi_type_check(navi_car(args), NAVI_STRING, env);

	if ((f = fopen((char*)navi_string(navi_car(args))->data, "r")) == NULL) {
		navi_file_error(env, "unable to open file");
	}

	return navi_make_file_input_port(f);
}

DEFUN(open_output_file, args, env, "open-output-file", 1, 0, NAVI_STRING)
{
	FILE *f;

	navi_type_check(navi_car(args), NAVI_STRING, env);

	if ((f = fopen((char*)navi_string(navi_car(args))->data, "w")) == NULL) {
		navi_file_error(env, "unable to open file");
	}

	return navi_make_file_output_port(f);
}

DEFUN(open_input_string, args, env, "open-input-string", 1, 0, NAVI_STRING)
{
	return make_string_input_port(navi_type_check(navi_car(args), NAVI_STRING, env));
}

DEFUN(open_output_string, args, env, "open-output-string", 0, 0)
{
	return make_string_output_port();
}

DEFUN(get_output_string, args, env, "get-output-string", 1, 0, NAVI_PORT)
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

DEFUN(close_port, args, env, "close-port", 1, 0, NAVI_PORT)
{
	struct navi_port *p = navi_port_cast(navi_car(args), env);
	if (p->read_u8 || p->read_char)
		close_input_port(p, env);
	if (p->write_u8 || p->write_char)
		close_output_port(p, env);
	return navi_unspecified();
}

DEFUN(close_input_port, args, env, "close-input-port", 1, 0, NAVI_PORT)
{
	struct navi_port *p = navi_port_cast(navi_car(args), env);
	check_input_port(p, env);
	close_input_port(p, env);
	return navi_unspecified();
}

DEFUN(close_output_port, args, env, "close-output-port", 1, 0, NAVI_PORT)
{
	struct navi_port *p = navi_port_cast(navi_car(args), env);
	check_output_port(p, env);
	close_output_port(p, env);
	return navi_unspecified();
}

DEFUN(eof_objectp, args, env, "eof-object?", 1, 0, NAVI_ANY)
{
	return navi_make_bool(navi_is_eof(navi_car(args)));
}

DEFUN(eof_object, args, env, "eof-object", 0, 0)
{
	return navi_make_eof();
}

DEFUN(read_u8, args, env, "read-u8", 0, NAVI_PROC_VARIADIC)
{
	struct navi_port *p = get_input_port(args, env);
	return navi_port_read_byte(p, env);
}

DEFUN(peek_u8, args, env, "peek-u8", 0, NAVI_PROC_VARIADIC)
{
	struct navi_port *p = get_input_port(args, env);
	return navi_port_peek_byte(p, env);
}

DEFUN(read_char, args, env, "read-char", 0, NAVI_PROC_VARIADIC)
{
	struct navi_port *p = get_input_port(args, env);
	return navi_port_read_char(p, env);
}

DEFUN(peek_char, args, env, "peek-char", 0, NAVI_PROC_VARIADIC)
{
	struct navi_port *p = get_input_port(args, env);
	return navi_port_peek_char(p, env);
}

DEFUN(read, args, env, "read", 0, NAVI_PROC_VARIADIC)
{
	struct navi_port *p = get_input_port(args, env);
	return navi_read(p, env);
}

DEFUN(write_u8, args, env, "write-u8", 1, NAVI_PROC_VARIADIC, NAVI_NUM)
{
	struct navi_port *p = get_output_port(navi_cdr(args), env);
	navi_port_write_char(navi_type_check_byte(navi_car(args), env), p, env);
	return navi_unspecified();
}

DEFUN(write_char, args, env, "write-char", 1, NAVI_PROC_VARIADIC, NAVI_CHAR)
{
	struct navi_port *p = get_output_port(navi_cdr(args), env);
	navi_port_write_char(navi_char_cast(navi_car(args), env), p, env);
	return navi_unspecified();
}

DEFUN(write_string, args, env, "write-string", 1, NAVI_PROC_VARIADIC, NAVI_STRING)
{
	struct navi_port *p = get_output_port(navi_cdr(args), env);
	navi_type_check(navi_car(args), NAVI_STRING, env);
	navi_port_write_cstr((char*)navi_string(navi_car(args))->data, p, env);
	return navi_unspecified();
}

DEFUN(display, args, env, "display", 1, NAVI_PROC_VARIADIC, NAVI_ANY)
{
	struct navi_port *p = get_output_port(navi_cdr(args), env);
	navi_port_display(p, navi_car(args), env);
	return navi_unspecified();
}

DEFUN(write, args, env, "write", 1, NAVI_PROC_VARIADIC, NAVI_ANY)
{
	struct navi_port *p = get_output_port(navi_cdr(args), env);
	navi_port_write(p, navi_car(args), env);
	return navi_unspecified();
}

DEFUN(newline, args, env, "newline", 0, NAVI_PROC_VARIADIC)
{
	scm_write_u8(navi_make_pair(navi_make_char('\n'), args), env);
	return navi_unspecified();
}
