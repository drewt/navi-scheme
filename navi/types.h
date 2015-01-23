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

#ifndef _NAVI_TYPES_H
#define _NAVI_TYPES_H

#include <stdbool.h>
#include <setjmp.h>
#include <stdio.h>

#include "clist.h"
#include "uchar.h"

#ifdef NAVI_COMPILE
#define SEXP_PREFIX(name) name
#else
#define SEXP_PREFIX(name) sexp_##name
#endif

#define ENV_HT_SIZE 64

#define IMMEDIATE_TAG_SIZE 0x4
#define IMMEDIATE_TAG_MASK 0xF
#define VOID_TAG 0x0L
#define BOOL_TAG 0x2L
#define CHAR_TAG 0x6L
#define NIL_TAG  0xAL
#define EOF_TAG  0xEL

/* C types {{{ */

typedef union {
	long n;
	struct sexp *p;
} sexp_t;

struct sexp_scope {
	struct list_head chain;
	struct sexp_scope *next;
	unsigned int refs;
	struct hlist_head bindings[ENV_HT_SIZE];
};
typedef struct sexp_scope *env_t;

typedef sexp_t (*builtin_t)(sexp_t,env_t);
typedef sexp_t(*sexp_leaf_t)(sexp_t, void*);

enum sexp_type {
	SEXP_VOID,
	SEXP_NIL,
	SEXP_NUM,
	SEXP_EOF,
	SEXP_BOOL,
	SEXP_CHAR,
	SEXP_PAIR,
	SEXP_PORT,
	SEXP_STRING,
	SEXP_SYMBOL,
	SEXP_VECTOR,
	SEXP_BYTEVEC,
	SEXP_VALUES,
	SEXP_MACRO,
	SEXP_SPECIAL,
	SEXP_PROMISE,
	SEXP_FUNCTION,
	SEXP_CASELAMBDA,
	SEXP_ESCAPE,
	SEXP_BOUNCE,
	SEXP_ENVIRONMENT,
};

struct sexp_escape {
	jmp_buf state;
	env_t env;
	sexp_t arg;
};

struct sexp_function {
	union {
		sexp_t body;
		builtin_t fn;
	};
	sexp_t args;
	char *name;
	env_t env;
	unsigned short arity;
	bool variadic;
	bool builtin;
};

struct sexp_vector {
	size_t size;
	sexp_t data[];
};

struct sexp_bytevec {
	size_t size;
	unsigned char data[];
};

struct sexp_string {
	size_t storage;
	size_t size;
	size_t length;
	char *data;
};

struct sexp_pair {
	sexp_t car;
	sexp_t cdr;
};

struct sexp_port {
	int (*read_u8)(struct sexp_port*, env_t);
	void (*write_u8)(unsigned char, struct sexp_port*, env_t);
	void (*close_in)(struct sexp_port*, env_t);
	void (*close_out)(struct sexp_port*, env_t);
	unsigned long flags;
	uchar buffer;
	sexp_t sexp;
	size_t pos;
	void *specific;
};

union sexp_object {
	struct sexp_scope *env;
	struct sexp_escape esc;
	struct sexp_function fun;
	struct sexp_vector vec;
	struct sexp_bytevec bvec;
	struct sexp_string str;
	struct sexp_pair pair;
	struct sexp_port port;
};

struct sexp {
	struct list_head chain;
	enum sexp_type type;
	bool gc_mark;
	union sexp_object data[];
};

struct sexp_spec {
	enum sexp_type type;
	union {
		long num;
		char *str;
		struct sexp_function fun;
		struct sexp_vector vec;
		struct sexp_pair pair;
	};
	size_t size;
	const char *ident;
};

struct sexp_binding {
	struct hlist_node chain;
	sexp_t symbol;
	sexp_t object;
};

/* C types }}} */

void scope_free(env_t scope);

static inline void scope_unref(env_t env)
{
	if (--env->refs == 0)
		scope_free(env);
}

static inline void scope_ref(env_t env)
{
	env->refs++;
}

/* Accessors {{{ */

static inline long sexp_num(sexp_t sexp)
{
	return sexp.n >> 1;
}

static inline bool sexp_bool(sexp_t sexp)
{
	return sexp.n >> IMMEDIATE_TAG_SIZE;
}

static inline unsigned long sexp_char(sexp_t sexp)
{
	return sexp.n >> IMMEDIATE_TAG_SIZE;
}

static inline struct sexp *sexp_ptr(sexp_t sexp)
{
	return sexp.p;
}

static inline struct sexp_vector *sexp_vector(sexp_t sexp)
{
	return &sexp.p->data->vec;
}

static inline struct sexp_bytevec *sexp_bytevec(sexp_t sexp)
{
	return &sexp.p->data->bvec;
}

static inline struct sexp_string *sexp_string(sexp_t sexp)
{
	return &sexp.p->data->str;
}

static inline struct sexp_function *sexp_fun(sexp_t sexp)
{
	return &sexp.p->data->fun;
}

static inline struct sexp_escape *sexp_escape(sexp_t sexp)
{
	return &sexp.p->data->esc;
}

static inline struct sexp_port *sexp_port(sexp_t sexp)
{
	return &sexp.p->data->port;
}

static inline env_t sexp_env(sexp_t sexp)
{
	return sexp.p->data->env;
}

static inline struct sexp_pair *sexp_pair(sexp_t sexp)
{
	return &sexp.p->data->pair;
}

static inline sexp_t SEXP_PREFIX(car)(sexp_t sexp)
{
	return sexp_pair(sexp)->car;
}

static inline sexp_t SEXP_PREFIX(cdr)(sexp_t sexp)
{
	return sexp_pair(sexp)->cdr;
}

static inline sexp_t SEXP_PREFIX(caar)(sexp_t sexp)
{
	return sexp_pair(car(sexp))->car;
}

static inline sexp_t SEXP_PREFIX(cadr)(sexp_t sexp)
{
	return sexp_pair(cdr(sexp))->car;
}

static inline sexp_t SEXP_PREFIX(cdar)(sexp_t sexp)
{
	return sexp_pair(car(sexp))->cdr;
}

static inline sexp_t SEXP_PREFIX(cddr)(sexp_t sexp)
{
	return sexp_pair(cdr(sexp))->cdr;
}

static inline sexp_t SEXP_PREFIX(caddr)(sexp_t sexp)
{
	return sexp_pair(cddr(sexp))->car;
}

static inline sexp_t SEXP_PREFIX(cadar)(sexp_t sexp)
{
	return sexp_pair(cdar(sexp))->car;
}

static inline sexp_t SEXP_PREFIX(cdddr)(sexp_t sexp)
{
	return sexp_pair(cddr(sexp))->cdr;
}

static inline sexp_t SEXP_PREFIX(cadddr)(sexp_t sexp)
{
	return sexp_pair(cdddr(sexp))->car;
}

static inline sexp_t SEXP_PREFIX(cddddr)(sexp_t sexp)
{
	return sexp_pair(cdddr(sexp))->cdr;
}

static inline sexp_t SEXP_PREFIX(caddddr)(sexp_t sexp)
{
	return sexp_pair(cddddr(sexp))->car;
}

/* Accessors }}} */

static inline enum sexp_type sexp_immediate_type(sexp_t val)
{
	unsigned long tag = val.n & IMMEDIATE_TAG_MASK;
	switch (tag) {
	case BOOL_TAG: return SEXP_BOOL;
	case CHAR_TAG: return SEXP_CHAR;
	case NIL_TAG:  return SEXP_NIL;
	case EOF_TAG:  return SEXP_EOF;
	}
	return -1;
}

static inline enum sexp_type sexp_type(sexp_t val)
{
	if (val.n == 0)
		return SEXP_VOID;
	if (val.n & 1)
		return SEXP_NUM;
	if (val.n & 2)
		return sexp_immediate_type(val);
	return val.p->type;
}

static inline bool sexp_ptr_type(sexp_t val)
{
	return val.n != 0 && (val.n & 3) == 0;
}

static inline sexp_t sexp_last_cons(sexp_t list)
{
	while (sexp_type(cdr(list)) == SEXP_PAIR)
		list = cdr(list);
	return list;
}

sexp_t port_read_byte(struct sexp_port *port, env_t env);
sexp_t port_peek_byte(struct sexp_port *port, env_t env);
sexp_t port_read_char(struct sexp_port *port, env_t env);
sexp_t port_peek_char(struct sexp_port *port, env_t env);
void port_write_byte(unsigned char ch, struct sexp_port *port, env_t env);
void port_write_char(uchar ch, struct sexp_port *port, env_t env);
void port_write_c_string(const char *str, struct sexp_port *port, env_t env);

/* conversion */
sexp_t list_to_vector(sexp_t list);
sexp_t vector_to_list(sexp_t vector);
sexp_t list_to_bytevec(sexp_t list, env_t env);
sexp_t bytevec_to_list(sexp_t sexp);
char *scm_to_c_string(sexp_t string);
char *bytevec_to_c_string(sexp_t sexp);
sexp_t string_to_bytevec(sexp_t string);
sexp_t bytevec_to_string(sexp_t bytevec);

void navi_init(void);
void sexp_free(struct sexp *sexp);
struct sexp *make_sexp(enum sexp_type type, size_t size);

/* Constructors {{{ */

sexp_t sexp_from_c_string(const char *str);
sexp_t sexp_from_c_bytevec(const char *str);
sexp_t sexp_make_symbol(const char *sym);
sexp_t sexp_make_pair(sexp_t car, sexp_t cdr);
sexp_t sexp_make_empty_pair(void);
sexp_t sexp_make_port(int(*read)(struct sexp_port*, env_t),
		void(*write)(unsigned char,struct sexp_port*, env_t),
		void(*close_in)(struct sexp_port*, env_t),
		void(*close_out)(struct sexp_port*, env_t),
		void *specific);
#define sexp_make_input_port(read, close, specific) \
	sexp_make_port(read, NULL, close, NULL, specific)
#define sexp_make_output_port(write, close, specific) \
	sexp_make_port(NULL, write, NULL, close, specific)
sexp_t sexp_make_file_input_port(FILE *file);
sexp_t sexp_make_file_output_port(FILE *file);
sexp_t sexp_make_vector(size_t size);
sexp_t sexp_make_bytevec(size_t size);
sexp_t sexp_make_string(size_t storage, size_t size, size_t length);
sexp_t sexp_make_function(sexp_t args, sexp_t body, char *name, env_t env);
sexp_t sexp_make_escape(void);

static inline sexp_t sexp_make_void(void)
{
	return (sexp_t) VOID_TAG;
}

static inline sexp_t sexp_make_nil(void)
{
	return (sexp_t) NIL_TAG;
}

static inline sexp_t sexp_make_eof(void)
{
	return (sexp_t) EOF_TAG;
}

static inline sexp_t sexp_make_num(long num)
{
	return (sexp_t) { .n = (num << 1) | 1 };
}

static inline sexp_t sexp_make_bool(bool b)
{
	return (sexp_t) { .n = (b << IMMEDIATE_TAG_SIZE) | BOOL_TAG };
}

static inline sexp_t sexp_make_char(unsigned long c)
{
	return (sexp_t) { .n = (c << IMMEDIATE_TAG_SIZE) | CHAR_TAG };
}

static inline sexp_t sexp_make_uninterned(const char *str)
{
	sexp_t sym = sexp_from_c_bytevec(str);
	sym.p->type = SEXP_SYMBOL;
	return sym;
}

static inline sexp_t sexp_make_macro(sexp_t args, sexp_t body, char *name, env_t env)
{
	sexp_t macro = sexp_make_function(args, body, name, env);
	macro.p->type = SEXP_MACRO;
	return macro;
}

static inline sexp_t sexp_make_promise(sexp_t e, env_t env)
{
	sexp_t body = sexp_make_pair(e, sexp_make_nil());
	sexp_t promise = sexp_make_function(sexp_make_nil(), body, "", env);
	promise.p->type = SEXP_PROMISE;
	return promise;
}

static inline sexp_t sexp_make_caselambda(size_t size)
{
	sexp_t lambda = sexp_make_vector(size);
	lambda.p->type = SEXP_CASELAMBDA;
	return lambda;
}

static inline sexp_t sexp_make_apair(const char *sym, sexp_t val)
{
	return sexp_make_pair(sexp_make_symbol(sym), val);
}

static inline sexp_t sexp_make_bounce(sexp_t object, sexp_t env)
{
	sexp_t ret = sexp_make_pair(object, env);
	ret.p->type = SEXP_BOUNCE;
	scope_ref(sexp_env(env));
	return ret;
}

/* Constructors }}} */

static inline sexp_t sexp_unspecified(void)
{
	return sexp_make_void();
}

static inline sexp_t vector_ref(sexp_t sexp, size_t i)
{
	return sexp_vector(sexp)->data[i];
}

static inline size_t vector_length(sexp_t sexp)
{
	return sexp.p->data->vec.size;
}

static inline sexp_t bytevec_ref(sexp_t sexp, size_t i)
{
	return sexp_make_num(sexp_bytevec(sexp)->data[i]);
}

static inline bool bytevec_equal(sexp_t sexp, const char *cstr)
{
	struct sexp_bytevec *vec = sexp_bytevec(sexp);
	for (size_t i = 0; i < vec->size; i++)
		if (vec->data[i] != (unsigned char) cstr[i])
			return false;
	return true;
}

static inline bool string_equal(sexp_t sexp, const char *cstr)
{
	struct sexp_string *string = sexp_string(sexp);
	for (size_t i = 0; i < string->size; i++)
		if (string->data[i] != cstr[i])
			return false;
	return true;
}

static inline bool sexp_string_equal(sexp_t a, sexp_t b)
{
	struct sexp_string *sa = sexp_string(a), *sb = sexp_string(b);
	if (sa->size != sb->size || sa->length != sb->length)
		return false;
	for (size_t i = 0; i < sa->size; i++)
		if (sa->data[i] != sb->data[i])
			return false;
	return true;
}

static inline const char *sexp_strtype(enum sexp_type type)
{
	switch (type) {
	case SEXP_VOID:        return "void";
	case SEXP_NIL:         return "nil";
	case SEXP_EOF:         return "eof-object";
	case SEXP_NUM:         return "number";
	case SEXP_BOOL:        return "boolean";
	case SEXP_CHAR:        return "character";
	case SEXP_VALUES:      return "values";
	case SEXP_PAIR:        return "pair";
	case SEXP_PORT:        return "port";
	case SEXP_STRING:      return "string";
	case SEXP_SYMBOL:      return "symbol";
	case SEXP_VECTOR:      return "vector";
	case SEXP_BYTEVEC:     return "bytevector";
	case SEXP_MACRO:       return "macro";
	case SEXP_SPECIAL:     return "special";
	case SEXP_PROMISE:     return "promise";
	case SEXP_FUNCTION:    return "function";
	case SEXP_CASELAMBDA:  return "case-lambda";
	case SEXP_ESCAPE:      return "escape";
	case SEXP_ENVIRONMENT: return "environment";
	case SEXP_BOUNCE:      return "bounce";
	}
	return "unknown";
}

static inline sexp_t sexp_typesym(enum sexp_type type)
{
	return sexp_make_symbol(sexp_strtype(type));
}

/* type predicates */

#define TYPE_PREDICATE(name, type) \
	static inline bool name(sexp_t sexp) \
	{ \
		return sexp_type(sexp) == type; \
	}

TYPE_PREDICATE(sexp_is_void, SEXP_VOID)
TYPE_PREDICATE(sexp_is_nil,  SEXP_NIL)
TYPE_PREDICATE(sexp_is_eof,  SEXP_EOF)
TYPE_PREDICATE(sexp_is_num,  SEXP_NUM)
TYPE_PREDICATE(sexp_is_bool, SEXP_BOOL)
TYPE_PREDICATE(sexp_is_char, SEXP_CHAR)
TYPE_PREDICATE(sexp_is_values, SEXP_VALUES)
TYPE_PREDICATE(sexp_is_pair, SEXP_PAIR)
TYPE_PREDICATE(sexp_is_string, SEXP_STRING)
TYPE_PREDICATE(sexp_is_symbol, SEXP_SYMBOL)
TYPE_PREDICATE(sexp_is_vector, SEXP_VECTOR)
TYPE_PREDICATE(sexp_is_bytevec, SEXP_BYTEVEC)
TYPE_PREDICATE(sexp_is_macro, SEXP_MACRO)
TYPE_PREDICATE(sexp_is_function, SEXP_FUNCTION)
TYPE_PREDICATE(sexp_is_caselambda, SEXP_CASELAMBDA)
TYPE_PREDICATE(sexp_is_escape, SEXP_ESCAPE)
TYPE_PREDICATE(sexp_is_environment, SEXP_ENVIRONMENT)
TYPE_PREDICATE(sexp_is_bounce, SEXP_BOUNCE)
#undef TYPE_PREDICATE

bool sexp_is_proper_list(sexp_t list);

#endif
