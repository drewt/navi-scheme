/* Copyright 2014-2015 Drew Thoreson
 *
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/.
 */

#ifndef _NAVI_EXTERN_H_
#define _NAVI_EXTERN_H_

#include <stdio.h>

#define NAVI_IMMEDIATE_TAG_BITS 0x4
#define NAVI_IMMEDIATE_TAG_MASK 0xF
#define NAVI_VOID_TAG 0x0L
#define NAVI_BOOL_TAG 0x2L
#define NAVI_CHAR_TAG 0x6L
#define NAVI_NIL_TAG  0xAL
#define NAVI_EOF_TAG  0xEL

struct navi_binding;
struct navi_scope;
struct navi_pair;
struct navi_port;
struct navi_string;
struct navi_symbol;
struct navi_vector;
struct navi_bytevec;
struct navi_procedure;
struct navi_thunk;
struct navi_escape;
struct navi_object;

typedef union {
	long n;
	struct navi_object *p;
	void *v;
} navi_obj;

typedef struct {
	struct navi_scope *lexical;
	struct navi_scope *dynamic;
} navi_env;

enum navi_type {
	NAVI_VOID,
	NAVI_NIL,
	NAVI_FIXNUM,
	NAVI_EOF,
	NAVI_BOOL,
	NAVI_CHAR,
	NAVI_PAIR,
	NAVI_PORT,
	NAVI_STRING,
	NAVI_SYMBOL,
	NAVI_VECTOR,
	NAVI_BYTEVEC,
	NAVI_VALUES,
	NAVI_MACRO,
	NAVI_SPECIAL,
	NAVI_PROMISE,
	NAVI_THUNK,
	NAVI_PROCEDURE,
	NAVI_CASELAMBDA,
	NAVI_ESCAPE,
	NAVI_PARAMETER,
	NAVI_BOUNCE,
	NAVI_ENVIRONMENT,
	NAVI_TRAP
};

enum {
	NAVI_LIST        = -1,
	NAVI_PROPER_LIST = -2,
	NAVI_BYTE        = -3,
	NAVI_ANY         = -4
};

void navi_init(void);

/* Memory Management {{{ */
void _navi_extern_scope_unref(struct navi_scope *scope);
#define _navi_scope_unref(scope) _navi_extern_scope_unref(scope)
struct navi_scope *_navi_extern_scope_ref(struct navi_scope *scope);
#define _navi_scope_ref(scope) _navi_extern_scope_ref(scope)
void navi_extern_env_ref(navi_env env);
#define _navi_env_ref(env) _navi_extern_env_ref(env)
/* Memory Management }}} */
/* Accessors {{{ */
#define navi_fixnum(obj) ((obj).n >> 1)
#define navi_bool(obj)   ((obj).n >> NAVI_IMMEDIATE_TAG_BITS)
#define navi_char(obj)   ((obj).n >> NAVI_IMMEDIATE_TAG_BITS)
struct navi_pair *navi_extern_pair(navi_obj obj);
#define navi_pair(obj) navi_extern_pair(obj)
struct navi_port *navi_extern_port(navi_obj obj);
#define navi_port(obj) navi_extern_port(obj)
struct navi_string *navi_extern_string(navi_obj obj);
#define navi_string(obj) navi_extern_string(obj)
struct navi_symbol *navi_extern_symbol(navi_obj obj);
#define navi_symbol(obj) navi_extern_symbol(obj)
struct navi_vector *navi_extern_vector(navi_obj obj);
#define navi_vector(obj) navi_extern_vector(obj)
struct navi_bytevec *navi_extern_bytevec(navi_obj obj);
#define navi_bytevec(obj) navi_extern_bytevec(obj)
struct navi_procedure *navi_extern_procedure(navi_obj obj);
#define navi_procedure(obj) navi_extern_procedure(obj)
struct navi_thunk *navi_extern_thunk(navi_obj obj);
#define navi_thunk(obj) navi_extern_thunk(obj)
struct navi_escape *navi_extern_escape(navi_obj obj);
#define navi_escape(obj) navi_extern_escape(obj)
navi_env navi_extern_environment(navi_obj obj);
#define navi_environment(obj) navi_extern_environment(obj)

navi_obj navi_extern_car(navi_obj obj);
#define navi_car(obj) navi_extern_car(obj)
navi_obj navi_extern_cdr(navi_obj obj);
#define navi_cdr(obj) navi_extern_cdr(obj)
/* Accessors }}} */
/* Constructors {{{ */
navi_obj navi_cstr_to_string(const char *str);
navi_obj navi_cstr_to_bytevec(const char *str);
navi_obj navi_make_symbol(const char *sym);
navi_obj navi_make_pair(navi_obj car, navi_obj cdr);
navi_obj navi_make_empty_pair(void);
navi_obj navi_make_port(
		int(*read_u8)(struct navi_port*, navi_env),
		void(*write_u8)(unsigned char,struct navi_port*, navi_env),
		long(*read_char)(struct navi_port*, navi_env),
		void(*write_char)(long, struct navi_port*, navi_env),
		void(*close_in)(struct navi_port*, navi_env),
		void(*close_out)(struct navi_port*, navi_env),
		void *specific);
#define navi_make_input_port(read_u8, read_char, close, specific) \
	navi_make_port(read_u8, NULL, read_char, NULL, close, NULL, specific)
#define navi_make_output_port(write_u8, write_char, close, specific) \
	navi_make_port(NULL, write_u8, NULL, write_char, NULL, close, specific)
#define navi_make_binary_input_port(read, close, specific) \
	navi_make_input_port(read, NULL, close, specific)
#define navi_make_binary_output_port(write, close, specific) \
	navi_make_output_port(write, NULL, close, specific)
#define navi_make_textual_input_port(read, close, specific) \
	navi_make_input_port(NULL, read, close, specific)
#define navi_make_textual_output_port(write, close, specific) \
	navi_make_output_port(NULL, write, close, specific)
navi_obj navi_make_file_input_port(FILE *file);
navi_obj navi_make_file_output_port(FILE *file);
navi_obj navi_make_vector(size_t size);
navi_obj navi_make_bytevec(size_t size);
navi_obj navi_make_string(size_t storage, size_t size, size_t length);
navi_obj navi_make_procedure(navi_obj args, navi_obj body, navi_obj name, navi_env env);
navi_obj navi_make_lambda(navi_obj args, navi_obj body, navi_env env);
navi_obj navi_make_thunk(navi_obj expr, navi_env env);
navi_obj navi_make_escape(void);
navi_obj _navi_make_parameter(navi_obj converter);
navi_obj navi_make_parameter(navi_obj value, navi_obj converter, navi_env env);
navi_obj _navi_make_named_parameter(navi_obj symbol, navi_obj converter);
navi_obj navi_make_named_parameter(navi_obj symbol, navi_obj value,
		navi_obj converter, navi_env env);

navi_obj navi_extern_make_void(void);
#define navi_make_void() navi_extern_make_void()
navi_obj navi_extern_make_nil(void);
#define navi_make_nil() navi_extern_make_nil()
navi_obj navi_extern_make_eof(void);
#define navi_make_eof() navi_extern_make_eof()
navi_obj navi_extern_make_fixnum(long num);
#define navi_make_fixnum(n) navi_extern_make_fixnum(n)
navi_obj navi_extern_make_bool(int b);
#define navi_make_bool(b) navi_extern_make_bool(b)
navi_obj navi_extern_make_char(unsigned long c);
#define navi_make_char(c) navi_extern_make_char(c)
/* Constructors }}} */
/* System Interface {{{ */
void navi_set_command_line(char *argv[], navi_env env);
/* System Interface }}} */
/* Environments/Evaluation {{{ */
void navi_add_lib_search_path(const char *path, navi_env env);
navi_obj navi_get_internal(navi_obj symbol, navi_env env);
navi_env navi_get_global_env(navi_env env);
struct navi_binding *navi_env_binding(struct navi_scope *env, navi_obj symbol);
void navi_scope_set(struct navi_scope *scope, navi_obj symbol, navi_obj object);
int navi_scope_unset(struct navi_scope *scope, navi_obj symbol);
navi_env navi_env_new_scope(navi_env env);
navi_env navi_dynamic_env_new_scope(navi_env env);
navi_env _navi_empty_environment(void);
navi_env navi_empty_environment(void);
navi_env _navi_interaction_environment(navi_env env);
navi_env navi_interaction_environment(void);
navi_obj navi_capture_env(navi_env env);
void navi_import(navi_obj imports, navi_env env);
navi_obj navi_eval(navi_obj expr, navi_env env);
navi_obj _navi_apply(struct navi_procedure *proc, navi_obj args, navi_env env);
navi_obj navi_call_escape(navi_obj escape, navi_obj arg, navi_env env);

navi_obj navi_extern_env_lookup(struct navi_scope *scope, navi_obj symbol);
#define navi_env_lookup(scope, symbol) navi_extern_env_lookup(scope, symbol)
navi_obj navi_extern_apply(struct navi_procedure *proc, navi_obj args,
		navi_env env);
#define navi_apply(proc, args, env) navi_extern_apply(proc, args, env)
/* Environments/Evaluation }}} */
/* Types {{{ */
enum navi_type navi_extern_type(navi_obj obj);
#define navi_type(obj) navi_extern_type(obj)
const char *navi_extern_strtype(enum navi_type type);
#define navi_strtype(type) navi_extern_strtype(obj)

int navi_extern_is_void(navi_obj obj);
#define navi_is_void(obj) navi_extern_is_void(obj)
int navi_extern_is_nil(navi_obj obj);
#define navi_is_nil(obj) navi_extern_is_nil(obj)
int navi_extern_is_eof(navi_obj obj);
#define navi_is_eof(obj) navi_extern_is_eof(obj)
int navi_extern_is_fixnum(navi_obj obj);
#define navi_is_fixnum(obj) navi_extern_is_fixnum(obj)
int navi_extern_is_bool(navi_obj obj);
#define navi_is_bool(obj) navi_extern_is_bool(obj)
int navi_extern_is_char(navi_obj obj);
#define navi_is_char(obj) navi_extern_is_char(obj)
int navi_extern_is_values(navi_obj obj);
#define navi_is_values(obj) navi_extern_is_values(obj)
int navi_extern_is_pair(navi_obj obj);
#define navi_is_pair(obj) navi_extern_is_pair(obj)
int navi_extern_is_port(navi_obj obj);
#define navi_is_port(obj) navi_extern_is_port(obj)
int navi_extern_is_string(navi_obj obj);
#define navi_is_string(obj) navi_extern_is_string(obj)
int navi_extern_is_symbol(navi_obj obj);
#define navi_is_symbol(obj) navi_extern_is_symbol(obj)
int navi_extern_is_vector(navi_obj obj);
#define navi_is_vector(obj) navi_extern_is_vector(obj)
int navi_extern_is_bytevec(navi_obj obj);
#define navi_is_bytevec(obj) navi_extern_is_bytevec(obj)
int navi_extern_is_macro(navi_obj obj);
#define navi_is_macro(obj) navi_extern_is_macro(obj)
int navi_extern_is_procedure(navi_obj obj);
#define navi_is_procedure(obj) navi_extern_is_procedure(obj)
int navi_extern_is_caselambda(navi_obj obj);
#define navi_is_caselambda(obj) navi_extern_is_caselambda(obj)
int navi_extern_is_escape(navi_obj obj);
#define navi_is_escape(obj) navi_extern_is_escape(obj)
int navi_extern_is_parameter(navi_obj obj);
#define navi_is_parameter(obj) navi_extern_is_parameter(obj)
int navi_extern_is_environment(navi_obj obj);
#define navi_is_environment(obj) navi_extern_is_environment(obj)
int navi_extern_is_byte(navi_obj obj);
#define navi_is_byte(obj) navi_extern_is_byte(obj)
int navi_extern_is_list(navi_obj obj);
#define navi_is_list(obj) navi_extern_is_list(obj)
int navi_is_proper_list(navi_obj list);
/* Types }}} */
/* Pairs/Lists {{{ */
int navi_list_length(navi_obj list);
navi_obj navi_list_append_ip(navi_obj a, navi_obj b);
navi_obj navi_list_tail(navi_obj list, long k);
navi_obj navi_list_copy(navi_obj list);

void navi_extern_set_car(navi_obj cons, navi_obj obj);
#define navi_set_car(cons, obj) navi_extern_set_car(cons, obj)
void navi_extern_set_cdr(navi_obj cons, navi_obj obj);
#define navi_set_cdr(cons, obj) navi_extern_set_cdr(cons, obj)

#define navi_list_for_each(cons, head)                                      \
	for (cons = (head); navi_type(cons) == NAVI_PAIR;                   \
			cons = navi_cdr(cons))

#define navi_list_for_each_safe(cons, n, head)                              \
	for (cons = (head),                                                 \
		n = (navi_type(head) == NAVI_PAIR) ? navi_cdr(head) : head; \
		navi_type(cons) == NAVI_PAIR;                               \
		cons = n,                                                   \
		n = (navi_type(n) == NAVI_PAIR) ? navi_cdr(n) : n)

#define navi_list_for_each_zipped(cons_a, cons_b, head_a, head_b)           \
	for (cons_a = (head_a), cons_b = (head_b);                          \
			navi_type(cons_a) == NAVI_PAIR &&                   \
			navi_type(cons_b) == NAVI_PAIR;                     \
			cons_a = navi_cdr(cons_a),                          \
			cons_b = navi_cdr(cons_b))
/* Pairs/Lists }}} */
/* Characters {{{ */
navi_obj navi_char_upcase(navi_obj ch);
navi_obj navi_char_downcase(navi_obj ch);
const char *navi_char_name(long value);
/* Characters }}} */
/* Ports {{{ */
int navi_port_is_fold_case(struct navi_port *port);
void navi_port_set_fold_case(struct navi_port *port, int fold);
navi_obj navi_read(struct navi_port *port, navi_env env);
void _navi_display(struct navi_port *port, navi_obj expr, int write, navi_env env);
navi_obj navi_port_read_byte(struct navi_port *port, navi_env env);
navi_obj navi_port_peek_byte(struct navi_port *port, navi_env env);
navi_obj navi_port_read_char(struct navi_port *port, navi_env env);
navi_obj navi_port_peek_char(struct navi_port *port, navi_env env);
void navi_port_write_byte(unsigned char ch, struct navi_port *port, navi_env env);
void navi_port_write_char(long ch, struct navi_port *port, navi_env env);
void navi_port_write_cstr(const char *str, struct navi_port *port, navi_env env);
navi_obj navi_current_input_port(navi_env env);
navi_obj navi_current_output_port(navi_env env);
navi_obj navi_current_error_port(navi_env env);
navi_obj _navi_open_input_file(const char *filename);
navi_obj _navi_open_output_file(const char *filename, navi_env env);
navi_obj navi_open_input_file(navi_obj filename, navi_env env);
navi_obj navi_open_output_file(navi_obj filename, navi_env env);
void navi_close_input_port(struct navi_port *p, navi_env env);
void navi_close_output_port(struct navi_port *p, navi_env env);
navi_obj navi_open_input_string(navi_obj string);
navi_obj navi_open_output_string(void);
navi_obj navi_get_output_string(navi_obj port);

#define navi_port_display(port, obj, env) _navi_display(port, obj, 0, env)
#define navi_port_write(port, obj, env)   _navi_display(port, obj, 1, env)
void navi_extern_display(navi_obj obj, navi_env env);
#define navi_display(obj, env) navi_extern_display(obj, env)
void navi_extern_write(navi_obj obj, navi_env env);
#define navi_write(obj, env) navi_extern_write(obj, env)
int navi_extern_port_is_input_port(struct navi_port *p);
#define navi_port_is_input_port(p) navi_extern_port_is_input_port(p)
int navi_extern_port_is_ouput_port(struct navi_port *p);
#define navi_port_is_output_port(p) navi_extern_port_is_ouput_port(p)
int navi_extern_is_input_port(navi_obj obj);
#define navi_is_input_port(obj) navi_extern_is_input_port(obj)
int navi_extern_is_output_port(navi_obj obj);
#define navi_is_output_port(obj) navi_extern_is_output_port(obj)
/* Ports }}} */
/* Strings {{{ */
navi_obj navi_string_copy(navi_obj str);
int navi_string_equal(navi_obj a, navi_obj b);
/* Vectors {{{ */
navi_obj navi_extern_vector_ref(navi_obj vec, size_t i);
#define navi_vector_ref(vec, i) navi_extern_vector_ref(vec, i)
size_t navi_extern_vector_length(navi_obj vec);
#define navi_vector_length(vec) navi_extern_vector_length(vec)
/* Vectors }}} */
/* Bytevectors {{{ */
navi_obj navi_extern_bytevec_ref(navi_obj vec, size_t i);
#define navi_bytevec_ref(vec, i) navi_extern_bytevec_ref(vec, i)
size_t navi_extern_bytevec_length(navi_obj vec);
#define navi_bytevec_length(vec) navi_extern_bytevec_length(vec)
/* Bytevectors }}} */
/* Parameters {{{ */
navi_obj navi_parameter_lookup(navi_obj param, navi_env env);
/* Parameters }}} */
/* Conversion {{{ */
navi_obj navi_list_to_vector(navi_obj list);
navi_obj navi_vector_to_list(navi_obj vector);
navi_obj navi_list_to_bytevec(navi_obj list, navi_env env);
char *navi_string_to_cstr(navi_obj string);
/* Conversion }}} */
/* Misc {{{ */
int navi_eqvp(navi_obj fst, navi_obj snd);
int navi_equalp(navi_obj fst, navi_obj snd);

#define navi_eqp(fst, snd) ((fst).p == (snd).p)
int navi_extern_is_true(navi_obj obj);
#define navi_is_true(obj) navi_extern_is_true(obj)
navi_obj navi_extern_force_tail(navi_obj obj, navi_env env);
#define navi_force_tail(obj, env) navi_extern_force_tail(obj, env)
/* Misc }}} */
#endif
