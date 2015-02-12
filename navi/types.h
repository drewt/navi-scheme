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

#ifndef _NAVI_TYPES_H
#define _NAVI_TYPES_H

#include <stdbool.h>
#include <setjmp.h>
#include <stdio.h>
#include <stdint.h>

#include "clist.h"

#define NAVI_ENV_HT_SIZE 64

#define NAVI_IMMEDIATE_TAG_BITS 0x4
#define NAVI_IMMEDIATE_TAG_MASK 0xF
#define NAVI_VOID_TAG 0x0L
#define NAVI_BOOL_TAG 0x2L
#define NAVI_CHAR_TAG 0x6L
#define NAVI_NIL_TAG  0xAL
#define NAVI_EOF_TAG  0xEL

/* C types {{{ */

typedef union {
	long n;
	struct navi_object *p;
} navi_obj;

struct navi_scope {
	struct navi_clist_head chain;
	struct navi_scope *next;
	unsigned int refs;
	struct navi_hlist_head bindings[NAVI_ENV_HT_SIZE];
};
typedef struct navi_scope *navi_env;

typedef navi_obj (*navi_builtin)(navi_obj,navi_env);
typedef navi_obj (*navi_leaf)(navi_obj, void*);

enum navi_type {
	NAVI_VOID,
	NAVI_NIL,
	NAVI_NUM,
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
	NAVI_PROCEDURE,
	NAVI_CASELAMBDA,
	NAVI_ESCAPE,
	NAVI_BOUNCE,
	NAVI_ENVIRONMENT,
};

enum {
	NAVI_LIST,
	NAVI_BYTE,
	NAVI_ANY,
};

struct navi_escape {
	jmp_buf state;
	navi_env env;
	navi_obj arg;
};

enum {
	NAVI_PROC_BUILTIN  = 1,
	NAVI_PROC_VARIADIC = 2,
};

struct navi_procedure {
	uint32_t flags;
	unsigned arity;
	navi_obj name;
	navi_env env;
	union {
		struct {
			navi_obj args;
			navi_obj body;
		};
		navi_builtin c_proc;
	};
};

struct navi_vector {
	size_t size;
	navi_obj data[];
};

struct navi_bytevec {
	size_t size;
	unsigned char data[];
};

struct navi_string {
	int32_t size;     // the number of bytes used by the string
	int32_t length;   // the number of code points encoded by the string
	int32_t capacity; // the available capacity (in bytes) of the string
	unsigned char *data;       // the UTF-8 encoded string data
};

struct navi_symbol {
	struct navi_hlist_node chain;
	char data[];
};

struct navi_pair {
	navi_obj car;
	navi_obj cdr;
};

struct navi_port {
	int (*read_u8)(struct navi_port*, navi_env);
	void (*write_u8)(uint8_t, struct navi_port*, navi_env);
	int32_t (*read_char)(struct navi_port*, navi_env);
	void (*write_char)(int32_t, struct navi_port*, navi_env);
	void (*close_in)(struct navi_port*, navi_env);
	void (*close_out)(struct navi_port*, navi_env);
	unsigned long flags;
	int32_t buffer;
	navi_obj expr;
	int32_t pos;
	void *specific;
};

struct navi_object {
	struct navi_clist_head chain;
	enum navi_type type;
	bool gc_mark;
	union {
		struct navi_scope *env;
		struct navi_escape esc;
		struct navi_procedure proc;
		struct navi_vector vec;
		struct navi_bytevec bvec;
		struct navi_string str;
		struct navi_symbol sym;
		struct navi_pair pair;
		struct navi_port port;
	} data[];
};

struct navi_spec {
	enum navi_type type;
	union {
		long num;
		char *str;
		struct navi_procedure proc;
		struct navi_vector vec;
		struct navi_pair pair;
	};
	size_t size;
	const char *ident;
};

struct navi_binding {
	struct navi_hlist_node chain;
	navi_obj symbol;
	navi_obj object;
};

/* C types }}} */

#define navi_die(msg, ...) \
	_navi_die("libnavi: critical error in " __FILE__ " (%d): " msg, \
			__LINE__, ##__VA_ARGS__)
static inline _Noreturn int _navi_die(const char *msg, ...)
{
	va_list ap;
	va_start(ap, msg);
	vfprintf(stderr, msg, ap);
	va_end(ap);
	putchar('\n');
	exit(1);
}

static inline void *navi_critical_malloc(size_t size)
{
	void *r;
	if (!(r = malloc(size)))
		navi_die("not enough memory");
	return r;
}

static inline void *navi_critical_realloc(void *p, size_t size)
{
	void *r;
	if (!(r = realloc(p, size)))
		navi_die("not enough memory");
	return r;
}

void navi_init(void);

/* Memory Management {{{ */
void navi_free(struct navi_object *obj);
void navi_scope_free(navi_env scope);

static inline void navi_scope_unref(navi_env env)
{
	if (--env->refs == 0)
		navi_scope_free(env);
}

static inline void navi_scope_ref(navi_env env)
{
	env->refs++;
}
/* Memory Management }}} */
/* Accessors {{{ */

static inline long navi_num(navi_obj obj)
{
	return obj.n >> 1;
}

static inline bool navi_bool(navi_obj obj)
{
	return obj.n >> NAVI_IMMEDIATE_TAG_BITS;
}

static inline unsigned long navi_char(navi_obj obj)
{
	return obj.n >> NAVI_IMMEDIATE_TAG_BITS;
}

static inline struct navi_object *navi_ptr(navi_obj obj)
{
	return obj.p;
}

static inline struct navi_vector *navi_vector(navi_obj obj)
{
	return &obj.p->data->vec;
}

static inline struct navi_bytevec *navi_bytevec(navi_obj obj)
{
	return &obj.p->data->bvec;
}

static inline struct navi_string *navi_string(navi_obj obj)
{
	return &obj.p->data->str;
}

static inline struct navi_symbol *navi_symbol(navi_obj obj)
{
	return &obj.p->data->sym;
}

static inline struct navi_procedure *navi_procedure(navi_obj obj)
{
	return &obj.p->data->proc;
}

static inline struct navi_escape *navi_escape(navi_obj obj)
{
	return &obj.p->data->esc;
}

static inline struct navi_port *navi_port(navi_obj obj)
{
	return &obj.p->data->port;
}

static inline navi_env navi_environment(navi_obj obj)
{
	return obj.p->data->env;
}

static inline struct navi_pair *navi_pair(navi_obj obj)
{
	return &obj.p->data->pair;
}

static inline struct navi_object *navi_object(void *concrete)
{
	return container_of(concrete, struct navi_object, data);
}

static inline navi_obj navi_car(navi_obj obj)
{
	return navi_pair(obj)->car;
}

static inline navi_obj navi_cdr(navi_obj obj)
{
	return navi_pair(obj)->cdr;
}

static inline navi_obj navi_caar(navi_obj obj)
{
	return navi_pair(navi_car(obj))->car;
}

static inline navi_obj navi_cadr(navi_obj obj)
{
	return navi_pair(navi_cdr(obj))->car;
}

static inline navi_obj navi_cdar(navi_obj obj)
{
	return navi_pair(navi_car(obj))->cdr;
}

static inline navi_obj navi_cddr(navi_obj obj)
{
	return navi_pair(navi_cdr(obj))->cdr;
}

static inline navi_obj navi_caddr(navi_obj obj)
{
	return navi_pair(navi_cddr(obj))->car;
}

static inline navi_obj navi_cadar(navi_obj obj)
{
	return navi_pair(navi_cdar(obj))->car;
}

static inline navi_obj navi_cdddr(navi_obj obj)
{
	return navi_pair(navi_cddr(obj))->cdr;
}

static inline navi_obj navi_cadddr(navi_obj obj)
{
	return navi_pair(navi_cdddr(obj))->car;
}

static inline navi_obj navi_cddddr(navi_obj obj)
{
	return navi_pair(navi_cdddr(obj))->cdr;
}

static inline navi_obj navi_caddddr(navi_obj obj)
{
	return navi_pair(navi_cddddr(obj))->car;
}

/* Accessors }}} */
/* Constructors {{{ */
navi_obj navi_from_spec(struct navi_spec *spec);
navi_obj navi_cstr_to_string(const char *str);
navi_obj navi_cstr_to_bytevec(const char *str);
navi_obj navi_make_symbol(const char *sym);
navi_obj navi_make_pair(navi_obj car, navi_obj cdr);
navi_obj navi_make_empty_pair(void);
navi_obj navi_make_port(
		int(*read_u8)(struct navi_port*, navi_env),
		void(*write_u8)(unsigned char,struct navi_port*, navi_env),
		int32_t(*read_char)(struct navi_port*, navi_env),
		void(*write_char)(int32_t, struct navi_port*, navi_env),
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
void navi_string_grow_storage(struct navi_string *str, long need);
navi_obj navi_make_procedure(navi_obj args, navi_obj body, navi_obj name, navi_env env);
navi_obj navi_make_lambda(navi_obj args, navi_obj body, navi_env env);
navi_obj navi_make_escape(void);

static inline navi_obj navi_make_void(void)
{
	return (navi_obj) NAVI_VOID_TAG;
}

static inline navi_obj navi_make_nil(void)
{
	return (navi_obj) NAVI_NIL_TAG;
}

static inline navi_obj navi_make_eof(void)
{
	return (navi_obj) NAVI_EOF_TAG;
}

static inline navi_obj navi_make_num(long num)
{
	return (navi_obj) { .n = (num << 1) | 1 };
}

static inline navi_obj navi_make_bool(bool b)
{
	return (navi_obj) { .n = (b << NAVI_IMMEDIATE_TAG_BITS) | NAVI_BOOL_TAG };
}

static inline navi_obj navi_make_char(unsigned long c)
{
	return (navi_obj) { .n = (c << NAVI_IMMEDIATE_TAG_BITS) | NAVI_CHAR_TAG };
}

static inline navi_obj navi_make_macro(navi_obj args, navi_obj body, navi_obj name,
		navi_env env)
{
	navi_obj macro = navi_make_procedure(args, body, name, env);
	macro.p->type = NAVI_MACRO;
	return macro;
}

static inline navi_obj navi_make_promise(navi_obj e, navi_env env)
{
	navi_obj body = navi_make_pair(e, navi_make_nil());
	navi_obj promise = navi_make_procedure(navi_make_nil(), body,
			navi_make_symbol("promise"), env);
	promise.p->type = NAVI_PROMISE;
	return promise;
}

static inline navi_obj navi_make_caselambda(size_t size)
{
	navi_obj lambda = navi_make_vector(size);
	lambda.p->type = NAVI_CASELAMBDA;
	return lambda;
}

static inline navi_obj navi_make_apair(const char *sym, navi_obj val)
{
	return navi_make_pair(navi_make_symbol(sym), val);
}

static inline navi_obj navi_make_bounce(navi_obj object, navi_obj env)
{
	navi_obj ret = navi_make_pair(object, env);
	ret.p->type = NAVI_BOUNCE;
	navi_scope_ref(navi_environment(env));
	return ret;
}

static inline navi_obj navi_unspecified(void)
{
	return navi_make_void();
}

/* Constructors }}} */
/* Environments/Evaluation {{{ */
struct navi_binding *navi_env_binding(navi_env env, navi_obj symbol);
navi_env navi_env_new_scope(navi_env env);
void navi_scope_set(navi_env env, navi_obj symbol, navi_obj object);
int navi_scope_unset(navi_env env, navi_obj symbol);
navi_env navi_extend_environment(navi_env env, navi_obj vars, navi_obj args);
navi_env navi_make_default_environment(void);
navi_obj navi_capture_env(navi_env env);

static inline navi_obj navi_env_lookup(navi_env env, navi_obj symbol)
{
	struct navi_binding *binding = navi_env_binding(env, symbol);
	return binding == NULL ? navi_make_void() : binding->object;
}

navi_obj navi_eval(navi_obj expr, navi_env env);
navi_obj navi_apply(struct navi_procedure *proc, navi_obj args, navi_env env);
navi_obj navi_call_escape(navi_obj escape, navi_obj arg);
/* Environments/Evaluation }}} */
/* Types {{{ */
static inline enum navi_type navi_immediate_type(navi_obj obj)
{
	unsigned long tag = obj.n & NAVI_IMMEDIATE_TAG_MASK;
	switch (tag) {
	case NAVI_BOOL_TAG: return NAVI_BOOL;
	case NAVI_CHAR_TAG: return NAVI_CHAR;
	case NAVI_NIL_TAG:  return NAVI_NIL;
	case NAVI_EOF_TAG:  return NAVI_EOF;
	}
	return -1;
}

static inline enum navi_type navi_type(navi_obj obj)
{
	if (obj.n == 0)
		return NAVI_VOID;
	if (obj.n & 1)
		return NAVI_NUM;
	if (obj.n & 2)
		return navi_immediate_type(obj);
	return obj.p->type;
}

static inline bool navi_ptr_type(navi_obj obj)
{
	return obj.n != 0 && (obj.n & 3) == 0;
}

static inline const char *navi_strtype(enum navi_type type)
{
	switch (type) {
	case NAVI_VOID:        return "void";
	case NAVI_NIL:         return "nil";
	case NAVI_EOF:         return "eof-object";
	case NAVI_NUM:         return "number";
	case NAVI_BOOL:        return "boolean";
	case NAVI_CHAR:        return "character";
	case NAVI_VALUES:      return "values";
	case NAVI_PAIR:        return "pair";
	case NAVI_PORT:        return "port";
	case NAVI_STRING:      return "string";
	case NAVI_SYMBOL:      return "symbol";
	case NAVI_VECTOR:      return "vector";
	case NAVI_BYTEVEC:     return "bytevector";
	case NAVI_MACRO:       return "macro";
	case NAVI_SPECIAL:     return "special";
	case NAVI_PROMISE:     return "promise";
	case NAVI_PROCEDURE:   return "procedure";
	case NAVI_CASELAMBDA:  return "case-lambda";
	case NAVI_ESCAPE:      return "escape";
	case NAVI_ENVIRONMENT: return "environment";
	case NAVI_BOUNCE:      return "bounce";
	}
	return "unknown";
}

static inline navi_obj navi_typesym(enum navi_type type)
{
	return navi_make_symbol(navi_strtype(type));
}

#define NAVI_TYPE_PREDICATE(name, type) \
	static inline bool name(navi_obj obj) \
	{ \
		return navi_type(obj) == type; \
	}

NAVI_TYPE_PREDICATE(navi_is_void, NAVI_VOID)
NAVI_TYPE_PREDICATE(navi_is_nil,  NAVI_NIL)
NAVI_TYPE_PREDICATE(navi_is_eof,  NAVI_EOF)
NAVI_TYPE_PREDICATE(navi_is_num,  NAVI_NUM)
NAVI_TYPE_PREDICATE(navi_is_bool, NAVI_BOOL)
NAVI_TYPE_PREDICATE(navi_is_char, NAVI_CHAR)
NAVI_TYPE_PREDICATE(navi_is_values, NAVI_VALUES)
NAVI_TYPE_PREDICATE(navi_is_pair, NAVI_PAIR)
NAVI_TYPE_PREDICATE(navi_is_string, NAVI_STRING)
NAVI_TYPE_PREDICATE(navi_is_symbol, NAVI_SYMBOL)
NAVI_TYPE_PREDICATE(navi_is_vector, NAVI_VECTOR)
NAVI_TYPE_PREDICATE(navi_is_bytevec, NAVI_BYTEVEC)
NAVI_TYPE_PREDICATE(navi_is_macro, NAVI_MACRO)
NAVI_TYPE_PREDICATE(navi_is_procedure, NAVI_PROCEDURE)
NAVI_TYPE_PREDICATE(navi_is_caselambda, NAVI_CASELAMBDA)
NAVI_TYPE_PREDICATE(navi_is_escape, NAVI_ESCAPE)
NAVI_TYPE_PREDICATE(navi_is_environment, NAVI_ENVIRONMENT)
NAVI_TYPE_PREDICATE(navi_is_bounce, NAVI_BOUNCE)
#undef NAVI_TYPE_PREDICATE

static inline bool navi_is_builtin(navi_obj obj)
{
	return navi_is_procedure(obj) &&
		navi_procedure(obj)->flags & NAVI_PROC_BUILTIN;
}

bool navi_is_proper_list(navi_obj list);
/* Types }}} */
/* Procedures {{{ */
static inline bool navi_proc_is_builtin(struct navi_procedure *p)
{
	return p->flags & NAVI_PROC_BUILTIN;
}

static inline bool navi_proc_is_variadic(struct navi_procedure *p)
{
	return p->flags & NAVI_PROC_VARIADIC;
}

static inline bool navi_arity_satisfied(struct navi_procedure *p, unsigned n)
{
	return n == p->arity
		|| ((p->flags & NAVI_PROC_VARIADIC) && n > p->arity);
}
/* Procedures }}} */
/* Lists {{{ */
navi_obj navi_vlist(navi_obj first, va_list ap);
navi_obj navi_list(navi_obj first, ...);
navi_obj navi_map(navi_obj list, navi_leaf fn, void *data);

static inline navi_obj navi_last_cons(navi_obj list)
{
	while (navi_type(navi_cdr(list)) == NAVI_PAIR)
		list = navi_cdr(list);
	return list;
}

static inline bool navi_is_last_pair(navi_obj pair)
{
	return navi_type(navi_cdr(pair)) == NAVI_NIL;
}

static inline int navi_list_length(navi_obj list)
{
	int i;

	for (i = 0; navi_type(list) == NAVI_PAIR; i++)
		list = navi_cdr(list);

	if (navi_type(list) != NAVI_NIL)
		navi_die("navi_list_length: not a proper list");

	return i;
}
/* Lists }}} */
/* Characters {{{ */
navi_obj navi_char_upcase(navi_obj ch);
navi_obj navi_char_downcase(navi_obj ch);
/* Characters }}} */
/* Ports {{{ */
navi_obj navi_read(struct navi_port *port, navi_env env);
void _navi_display(struct navi_port *port, navi_obj expr, bool write, navi_env env);
navi_obj navi_port_read_byte(struct navi_port *port, navi_env env);
navi_obj navi_port_peek_byte(struct navi_port *port, navi_env env);
navi_obj navi_port_read_char(struct navi_port *port, navi_env env);
navi_obj navi_port_peek_char(struct navi_port *port, navi_env env);
void navi_port_write_byte(unsigned char ch, struct navi_port *port, navi_env env);
void navi_port_write_char(int32_t ch, struct navi_port *port, navi_env env);
void navi_port_write_cstr(const char *str, struct navi_port *port, navi_env env);
navi_obj navi_current_input_port(navi_env env);
navi_obj navi_current_output_port(navi_env env);
navi_obj navi_current_error_port(navi_env env);

static inline void navi_port_display(struct navi_port *port, navi_obj obj, navi_env env)
{
	_navi_display(port, obj, false, env);
}

static inline void navi_port_write(struct navi_port *port, navi_obj obj, navi_env env)
{
	_navi_display(port, obj, true, env);
}

static inline void navi_display(navi_obj obj, navi_env env)
{
	navi_port_display(navi_port(navi_current_output_port(env)), obj, env);
}

static inline void navi_write(navi_obj obj, navi_env env)
{
	navi_port_write(navi_port(navi_current_output_port(env)), obj, env);
}

/* Ports }}} */
/* Strings {{{ */
navi_obj navi_string_copy(navi_obj str);

static inline bool navi_string_equal(navi_obj a, navi_obj b)
{
	struct navi_string *sa = navi_string(a), *sb = navi_string(b);
	if (sa->size != sb->size || sa->length != sb->length)
		return false;
	for (int32_t i = 0; i < sa->size; i++)
		if (sa->data[i] != sb->data[i])
			return false;
	return true;
}
/* Strings }}} */
/* Vectors {{{ */
static inline navi_obj navi_vector_ref(navi_obj vec, size_t i)
{
	return navi_vector(vec)->data[i];
}

static inline size_t navi_vector_length(navi_obj vec)
{
	return vec.p->data->vec.size;
}

navi_obj navi_vector_map(navi_obj proc, navi_obj to, navi_obj from, navi_env env);
/* Vectors }}} */
/* Bytevectors {{{ */
static inline navi_obj navi_bytevec_ref(navi_obj vec, size_t i)
{
	return navi_make_num(navi_bytevec(vec)->data[i]);
}

static inline bool navi_bytevec_equal(navi_obj obj, const char *cstr)
{
	struct navi_bytevec *vec = navi_bytevec(obj);
	for (size_t i = 0; i < vec->size; i++)
		if (vec->data[i] != (unsigned char) cstr[i])
			return false;
	return true;
}
/* Bytevectors }}} */
/* Conversion {{{ */
navi_obj navi_list_to_vector(navi_obj list);
navi_obj navi_vector_to_list(navi_obj vector);
navi_obj navi_list_to_bytevec(navi_obj list, navi_env env);
char *navi_string_to_cstr(navi_obj string);
/* Conversion }}} */
/* Misc {{{ */
bool navi_eqvp(navi_obj fst, navi_obj snd);

static inline bool navi_is_true(navi_obj expr)
{
	return navi_type(expr) != NAVI_BOOL || navi_bool(expr);
}

static inline bool navi_symbol_eq(navi_obj expr, navi_obj symbol)
{
	return navi_is_symbol(expr) && expr.p == symbol.p;
}

/* Misc }}} */

#endif
