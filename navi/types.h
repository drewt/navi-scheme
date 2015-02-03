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

#include "clist.h"
#include "uchar.h"

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
} navi_t;

struct navi_scope {
	struct navi_clist_head chain;
	struct navi_scope *next;
	unsigned int refs;
	struct navi_hlist_head bindings[NAVI_ENV_HT_SIZE];
};
typedef struct navi_scope *navi_env_t;

typedef navi_t (*navi_builtin_t)(navi_t,navi_env_t);
typedef navi_t (*navi_leaf_t)(navi_t, void*);

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
	NAVI_FUNCTION,
	NAVI_CASELAMBDA,
	NAVI_ESCAPE,
	NAVI_BOUNCE,
	NAVI_ENVIRONMENT,
};

struct navi_escape {
	jmp_buf state;
	navi_env_t env;
	navi_t arg;
};

struct navi_function {
	union {
		navi_t body;
		navi_builtin_t fn;
	};
	navi_t args;
	char *name;
	navi_env_t env;
	unsigned short arity;
	bool variadic;
	bool builtin;
};

struct navi_vector {
	size_t size;
	navi_t data[];
};

struct navi_bytevec {
	size_t size;
	unsigned char data[];
};

struct navi_string {
	size_t storage;
	size_t size;
	size_t length;
	char *data;
};

struct navi_pair {
	navi_t car;
	navi_t cdr;
};

struct navi_port {
	int (*read_u8)(struct navi_port*, navi_env_t);
	void (*write_u8)(unsigned char, struct navi_port*, navi_env_t);
	void (*close_in)(struct navi_port*, navi_env_t);
	void (*close_out)(struct navi_port*, navi_env_t);
	unsigned long flags;
	uchar buffer;
	navi_t expr;
	size_t pos;
	void *specific;
};

struct navi_object {
	struct navi_clist_head chain;
	enum navi_type type;
	bool gc_mark;
	union {
		struct navi_scope *env;
		struct navi_escape esc;
		struct navi_function fun;
		struct navi_vector vec;
		struct navi_bytevec bvec;
		struct navi_string str;
		struct navi_pair pair;
		struct navi_port port;
	} data[];
};

struct navi_spec {
	enum navi_type type;
	union {
		long num;
		char *str;
		struct navi_function fun;
		struct navi_vector vec;
		struct navi_pair pair;
	};
	size_t size;
	const char *ident;
};

struct navi_binding {
	struct navi_hlist_node chain;
	navi_t symbol;
	navi_t object;
};

/* C types }}} */

void navi_scope_free(navi_env_t scope);

static inline void navi_scope_unref(navi_env_t env)
{
	if (--env->refs == 0)
		navi_scope_free(env);
}

static inline void navi_scope_ref(navi_env_t env)
{
	env->refs++;
}

/* Accessors {{{ */

static inline long navi_num(navi_t obj)
{
	return obj.n >> 1;
}

static inline bool navi_bool(navi_t obj)
{
	return obj.n >> NAVI_IMMEDIATE_TAG_BITS;
}

static inline unsigned long navi_char(navi_t obj)
{
	return obj.n >> NAVI_IMMEDIATE_TAG_BITS;
}

static inline struct navi_object *navi_ptr(navi_t obj)
{
	return obj.p;
}

static inline struct navi_vector *navi_vector(navi_t obj)
{
	return &obj.p->data->vec;
}

static inline struct navi_bytevec *navi_bytevec(navi_t obj)
{
	return &obj.p->data->bvec;
}

static inline struct navi_string *navi_string(navi_t obj)
{
	return &obj.p->data->str;
}

static inline struct navi_function *navi_fun(navi_t obj)
{
	return &obj.p->data->fun;
}

static inline struct navi_escape *navi_escape(navi_t obj)
{
	return &obj.p->data->esc;
}

static inline struct navi_port *navi_port(navi_t obj)
{
	return &obj.p->data->port;
}

static inline navi_env_t navi_env(navi_t obj)
{
	return obj.p->data->env;
}

static inline struct navi_pair *navi_pair(navi_t obj)
{
	return &obj.p->data->pair;
}

static inline navi_t navi_car(navi_t obj)
{
	return navi_pair(obj)->car;
}

static inline navi_t navi_cdr(navi_t obj)
{
	return navi_pair(obj)->cdr;
}

static inline navi_t navi_caar(navi_t obj)
{
	return navi_pair(navi_car(obj))->car;
}

static inline navi_t navi_cadr(navi_t obj)
{
	return navi_pair(navi_cdr(obj))->car;
}

static inline navi_t navi_cdar(navi_t obj)
{
	return navi_pair(navi_car(obj))->cdr;
}

static inline navi_t navi_cddr(navi_t obj)
{
	return navi_pair(navi_cdr(obj))->cdr;
}

static inline navi_t navi_caddr(navi_t obj)
{
	return navi_pair(navi_cddr(obj))->car;
}

static inline navi_t navi_cadar(navi_t obj)
{
	return navi_pair(navi_cdar(obj))->car;
}

static inline navi_t navi_cdddr(navi_t obj)
{
	return navi_pair(navi_cddr(obj))->cdr;
}

static inline navi_t navi_cadddr(navi_t obj)
{
	return navi_pair(navi_cdddr(obj))->car;
}

static inline navi_t navi_cddddr(navi_t obj)
{
	return navi_pair(navi_cdddr(obj))->cdr;
}

static inline navi_t navi_caddddr(navi_t obj)
{
	return navi_pair(navi_cddddr(obj))->car;
}

/* Accessors }}} */

static inline enum navi_type navi_immediate_type(navi_t obj)
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

static inline enum navi_type navi_type(navi_t obj)
{
	if (obj.n == 0)
		return NAVI_VOID;
	if (obj.n & 1)
		return NAVI_NUM;
	if (obj.n & 2)
		return navi_immediate_type(obj);
	return obj.p->type;
}

static inline bool navi_ptr_type(navi_t obj)
{
	return obj.n != 0 && (obj.n & 3) == 0;
}

static inline navi_t navi_last_cons(navi_t list)
{
	while (navi_type(navi_cdr(list)) == NAVI_PAIR)
		list = navi_cdr(list);
	return list;
}

navi_t navi_char_upcase(navi_t ch);
navi_t navi_char_downcase(navi_t ch);

navi_t navi_port_read_byte(struct navi_port *port, navi_env_t env);
navi_t navi_port_peek_byte(struct navi_port *port, navi_env_t env);
navi_t navi_port_read_char(struct navi_port *port, navi_env_t env);
navi_t navi_port_peek_char(struct navi_port *port, navi_env_t env);
void navi_port_write_byte(unsigned char ch, struct navi_port *port, navi_env_t env);
void navi_port_write_char(uchar ch, struct navi_port *port, navi_env_t env);
void navi_port_write_cstr(const char *str, struct navi_port *port, navi_env_t env);

/* conversion */
navi_t navi_list_to_vector(navi_t list);
navi_t navi_vector_to_list(navi_t vector);
navi_t navi_list_to_bytevec(navi_t list, navi_env_t env);
navi_t navi_bytevec_to_list(navi_t vec);
char *navi_string_to_cstr(navi_t string);
char *navi_bytevec_to_cstr(navi_t vec);
navi_t navi_string_to_bytevec(navi_t string);
navi_t navi_bytevec_to_string(navi_t bytevec);

void navi_init(void);
void navi_free(struct navi_object *obj);

/* Constructors {{{ */

navi_t navi_cstr_to_string(const char *str);
navi_t navi_cstr_to_bytevec(const char *str);
navi_t navi_make_symbol(const char *sym);
navi_t navi_make_pair(navi_t car, navi_t cdr);
navi_t navi_make_empty_pair(void);
navi_t navi_make_port(int(*read)(struct navi_port*, navi_env_t),
		void(*write)(unsigned char,struct navi_port*, navi_env_t),
		void(*close_in)(struct navi_port*, navi_env_t),
		void(*close_out)(struct navi_port*, navi_env_t),
		void *specific);
#define navi_make_input_port(read, close, specific) \
	navi_make_port(read, NULL, close, NULL, specific)
#define navi_make_output_port(write, close, specific) \
	navi_make_port(NULL, write, NULL, close, specific)
navi_t navi_make_file_input_port(FILE *file);
navi_t navi_make_file_output_port(FILE *file);
navi_t navi_make_vector(size_t size);
navi_t navi_make_bytevec(size_t size);
navi_t navi_make_string(size_t storage, size_t size, size_t length);
navi_t navi_make_function(navi_t args, navi_t body, char *name, navi_env_t env);
navi_t navi_make_escape(void);

static inline navi_t navi_make_void(void)
{
	return (navi_t) NAVI_VOID_TAG;
}

static inline navi_t navi_make_nil(void)
{
	return (navi_t) NAVI_NIL_TAG;
}

static inline navi_t navi_make_eof(void)
{
	return (navi_t) NAVI_EOF_TAG;
}

static inline navi_t navi_make_num(long num)
{
	return (navi_t) { .n = (num << 1) | 1 };
}

static inline navi_t navi_make_bool(bool b)
{
	return (navi_t) { .n = (b << NAVI_IMMEDIATE_TAG_BITS) | NAVI_BOOL_TAG };
}

static inline navi_t navi_make_char(unsigned long c)
{
	return (navi_t) { .n = (c << NAVI_IMMEDIATE_TAG_BITS) | NAVI_CHAR_TAG };
}

static inline navi_t navi_make_uninterned(const char *str)
{
	navi_t sym = navi_cstr_to_bytevec(str);
	sym.p->type = NAVI_SYMBOL;
	return sym;
}

static inline navi_t navi_make_macro(navi_t args, navi_t body, char *name, navi_env_t env)
{
	navi_t macro = navi_make_function(args, body, name, env);
	macro.p->type = NAVI_MACRO;
	return macro;
}

static inline navi_t navi_make_promise(navi_t e, navi_env_t env)
{
	navi_t body = navi_make_pair(e, navi_make_nil());
	navi_t promise = navi_make_function(navi_make_nil(), body, "", env);
	promise.p->type = NAVI_PROMISE;
	return promise;
}

static inline navi_t navi_make_caselambda(size_t size)
{
	navi_t lambda = navi_make_vector(size);
	lambda.p->type = NAVI_CASELAMBDA;
	return lambda;
}

static inline navi_t navi_make_apair(const char *sym, navi_t val)
{
	return navi_make_pair(navi_make_symbol(sym), val);
}

static inline navi_t navi_make_bounce(navi_t object, navi_t env)
{
	navi_t ret = navi_make_pair(object, env);
	ret.p->type = NAVI_BOUNCE;
	navi_scope_ref(navi_env(env));
	return ret;
}

/* Constructors }}} */

static inline navi_t navi_unspecified(void)
{
	return navi_make_void();
}

static inline navi_t navi_vector_ref(navi_t vec, size_t i)
{
	return navi_vector(vec)->data[i];
}

static inline size_t navi_vector_length(navi_t vec)
{
	return vec.p->data->vec.size;
}

static inline navi_t navi_bytevec_ref(navi_t vec, size_t i)
{
	return navi_make_num(navi_bytevec(vec)->data[i]);
}

static inline bool navi_bytevec_equal(navi_t obj, const char *cstr)
{
	struct navi_bytevec *vec = navi_bytevec(obj);
	for (size_t i = 0; i < vec->size; i++)
		if (vec->data[i] != (unsigned char) cstr[i])
			return false;
	return true;
}

static inline bool navi_string_equal(navi_t a, navi_t b)
{
	struct navi_string *sa = navi_string(a), *sb = navi_string(b);
	if (sa->size != sb->size || sa->length != sb->length)
		return false;
	for (size_t i = 0; i < sa->size; i++)
		if (sa->data[i] != sb->data[i])
			return false;
	return true;
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
	case NAVI_FUNCTION:    return "function";
	case NAVI_CASELAMBDA:  return "case-lambda";
	case NAVI_ESCAPE:      return "escape";
	case NAVI_ENVIRONMENT: return "environment";
	case NAVI_BOUNCE:      return "bounce";
	}
	return "unknown";
}

static inline navi_t navi_typesym(enum navi_type type)
{
	return navi_make_symbol(navi_strtype(type));
}

/* type predicates */

#define TYPE_PREDICATE(name, type) \
	static inline bool name(navi_t obj) \
	{ \
		return navi_type(obj) == type; \
	}

TYPE_PREDICATE(navi_is_void, NAVI_VOID)
TYPE_PREDICATE(navi_is_nil,  NAVI_NIL)
TYPE_PREDICATE(navi_is_eof,  NAVI_EOF)
TYPE_PREDICATE(navi_is_num,  NAVI_NUM)
TYPE_PREDICATE(navi_is_bool, NAVI_BOOL)
TYPE_PREDICATE(navi_is_char, NAVI_CHAR)
TYPE_PREDICATE(navi_is_values, NAVI_VALUES)
TYPE_PREDICATE(navi_is_pair, NAVI_PAIR)
TYPE_PREDICATE(navi_is_string, NAVI_STRING)
TYPE_PREDICATE(navi_is_symbol, NAVI_SYMBOL)
TYPE_PREDICATE(navi_is_vector, NAVI_VECTOR)
TYPE_PREDICATE(navi_is_bytevec, NAVI_BYTEVEC)
TYPE_PREDICATE(navi_is_macro, NAVI_MACRO)
TYPE_PREDICATE(navi_is_function, NAVI_FUNCTION)
TYPE_PREDICATE(navi_is_caselambda, NAVI_CASELAMBDA)
TYPE_PREDICATE(navi_is_escape, NAVI_ESCAPE)
TYPE_PREDICATE(navi_is_environment, NAVI_ENVIRONMENT)
TYPE_PREDICATE(navi_is_bounce, NAVI_BOUNCE)
#undef TYPE_PREDICATE

bool navi_is_proper_list(navi_t list);

#endif
