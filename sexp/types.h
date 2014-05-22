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

#ifndef _SEXP_TYPES_H
#define _SEXP_TYPES_H

#include <setjmp.h>

#include "clist.h"
#include "uchar.h"

#define ENV_HT_SIZE 64

#define IMMEDIATE_TAG_SIZE 0x4
#define IMMEDIATE_TAG_MASK 0xF
#define VOID_TAG 0x0UL
#define BOOL_TAG 0x2UL
#define CHAR_TAG 0x6UL
#define NIL_TAG  0xAUL
#define EOF_TAG  0xEUL

typedef union {
	unsigned long n;
	struct sexp *p;
} sexp_t;

struct sexp_scope {
	struct list_head chain;
	struct sexp_scope *next;
	unsigned int refs;
	struct hlist_head bindings[ENV_HT_SIZE];
};
typedef struct sexp_scope *env_t;


enum sexp_type {
	SEXP_VOID,
	SEXP_NIL,
	SEXP_NUM,
	SEXP_EOF,
	SEXP_BOOL,
	SEXP_CHAR,
	SEXP_PAIR,
	SEXP_STRING,
	SEXP_SYMBOL,
	SEXP_VECTOR,
	SEXP_BYTEVEC,
	SEXP_VALUES,
	SEXP_MACRO,
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
		sexp_t (*fn)(sexp_t,struct sexp_scope*);
	};
	sexp_t args;
	char *name;
	env_t env;
	unsigned short arity;
	char variadic;
	char builtin;
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
	size_t size;
	size_t length;
	char *data;
};

struct sexp_pair {
	sexp_t car;
	sexp_t cdr;
};

union sexp_object {
	struct sexp_scope *env;
	struct sexp_escape esc;
	struct sexp_function fun;
	struct sexp_vector vec;
	struct sexp_bytevec bvec;
	struct sexp_string str;
	struct sexp_pair pair;
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

void free_scope(env_t scope);

static inline void scope_unref(env_t env)
{
	if (--env->refs == 0)
		free_scope(env);
}

static inline void scope_ref(env_t env)
{
	env->refs++;
}

/*
 * Accessors (not type safe!!!)
 */

static inline long sexp_num(sexp_t sexp)
{
	return sexp.n >> 1;
	//return (long) (((unsigned long)sexp) >> 1);
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

static inline env_t sexp_env(sexp_t sexp)
{
	return sexp.p->data->env;
}

static inline struct sexp_pair *sexp_pair(sexp_t sexp)
{
	return &sexp.p->data->pair;
}

static inline sexp_t car(sexp_t sexp)
{
	return sexp_pair(sexp)->car;
}

static inline sexp_t cdr(sexp_t sexp)
{
	return sexp_pair(sexp)->cdr;
}

static inline sexp_t caar(sexp_t sexp)
{
	return sexp_pair(car(sexp))->car;
}

static inline sexp_t cadr(sexp_t sexp)
{
	return sexp_pair(cdr(sexp))->car;
}

static inline sexp_t cdar(sexp_t sexp)
{
	return sexp_pair(car(sexp))->cdr;
}

static inline sexp_t cddr(sexp_t sexp)
{
	return sexp_pair(cdr(sexp))->cdr;
}

static inline sexp_t caddr(sexp_t sexp)
{
	return sexp_pair(cddr(sexp))->car;
}

static inline sexp_t cdddr(sexp_t sexp)
{
	return sexp_pair(cddr(sexp))->cdr;
}

static inline sexp_t cadddr(sexp_t sexp)
{
	return sexp_pair(cdddr(sexp))->car;
}

static inline sexp_t cddddr(sexp_t sexp)
{
	return sexp_pair(cdddr(sexp))->cdr;
}

static inline sexp_t caddddr(sexp_t sexp)
{
	return sexp_pair(cddddr(sexp))->car;
}

static inline void set_car(sexp_t cons, sexp_t val)
{
	sexp_pair(cons)->car = val;
}

static inline void set_cdr(sexp_t cons, sexp_t val)
{
	sexp_pair(cons)->cdr = val;
}

#define bounce_object car

static inline env_t bounce_env(sexp_t bounce)
{
	return cdr(bounce).p->data->env;
}

static inline sexp_t object_to_sexp(void *object)
{
	return (sexp_t) ((struct sexp*) ((size_t)object - offsetof(struct sexp, data)));
}

static inline enum sexp_type sexp_immediate_type(sexp_t val)
{
	unsigned long tag = val.n & IMMEDIATE_TAG_MASK;
	switch (tag) {
	case BOOL_TAG: return SEXP_BOOL;
	case CHAR_TAG: return SEXP_CHAR;
	case NIL_TAG:  return SEXP_NIL;
	case EOF_TAG:  return SEXP_EOF;
	}
	die("invalid tag: %x\n", tag);
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

static inline bool ptr_type(sexp_t val)
{
	return val.n != 0 && (val.n & 3) == 0;
}

static inline sexp_t last_cons(sexp_t list)
{
	while (sexp_type(cdr(list)) == SEXP_PAIR)
		list = cdr(list);
	return list;
}

/* conversion */
sexp_t list_to_vector(sexp_t list);
sexp_t vector_to_list(sexp_t vector);
sexp_t list_to_bytevec(sexp_t list, env_t env);
sexp_t bytevec_to_list(sexp_t sexp);
char *scm_to_c_string(sexp_t string);
char *bytevec_to_c_string(sexp_t sexp);
sexp_t string_to_bytevec(sexp_t string);
sexp_t bytevec_to_string(sexp_t bytevec);

void symbol_table_init(void);
void sexp_free(struct sexp *sexp);
struct sexp *make_sexp(enum sexp_type type, size_t size);

/* constructors */
sexp_t to_string(const char *str);
sexp_t to_bytevec(const char *str);
sexp_t make_symbol(const char *sym);
sexp_t make_pair(sexp_t car, sexp_t cdr);
sexp_t make_empty_pair(void);
sexp_t make_vector(size_t size);
sexp_t make_bytevec(size_t size);
sexp_t make_string(size_t size);
sexp_t make_function(sexp_t args, sexp_t body, char *name, env_t env);
sexp_t make_escape(void);
sexp_t capture_env(env_t env);
sexp_t sexp_from_spec(struct sexp_spec *spec);
bool eqvp(sexp_t fst, sexp_t snd);

static inline sexp_t make_uninterned(const char *str)
{
	sexp_t sym = to_bytevec(str);
	sym.p->type = SEXP_SYMBOL;
	return sym;
}

static inline sexp_t make_macro(sexp_t args, sexp_t body, char *name, env_t env)
{
	sexp_t macro = make_function(args, body, name, env);
	macro.p->type = SEXP_MACRO;
	return macro;
}

static inline sexp_t make_values(sexp_t values)
{
	values = list_to_vector(values);
	values.p->type = SEXP_VALUES;
	return values;
}

static inline sexp_t make_caselambda(size_t size)
{
	sexp_t lambda = make_vector(size);
	lambda.p->type = SEXP_CASELAMBDA;
	return lambda;
}

static inline sexp_t make_apair(const char *sym, sexp_t val)
{
	return make_pair(make_symbol(sym), val);
}

static inline sexp_t make_void(void)
{
	return (sexp_t) VOID_TAG;
}

static inline sexp_t make_nil(void)
{
	return (sexp_t) NIL_TAG;
}

static inline sexp_t make_eof(void)
{
	return (sexp_t) EOF_TAG;	
}

static inline sexp_t make_num(long num)
{
	return (sexp_t) { .n = (num << 1) | 1 };
}

static inline sexp_t make_bool(bool b)
{
	return (sexp_t) { .n = (b << IMMEDIATE_TAG_SIZE) | BOOL_TAG };
}

static inline sexp_t make_char(unsigned long c)
{
	return (sexp_t) { .n = (c << IMMEDIATE_TAG_SIZE) | CHAR_TAG };
}

static inline sexp_t make_bounce(sexp_t object, sexp_t env)
{
	sexp_t ret = make_pair(object, env);
	ret.p->type = SEXP_BOUNCE;
	scope_ref(sexp_env(env));
	return ret;
}

static inline sexp_t unspecified(void)
{
	return make_void();
}

static inline sexp_t vector_ref(sexp_t sexp, size_t i)
{
	return sexp_vector(sexp)->data[i];
}

static inline sexp_t bytevec_ref(sexp_t sexp, size_t i)
{
	return make_char(sexp_bytevec(sexp)->data[i]);
}

static inline bool bytevec_equal(sexp_t sexp, const char *cstr)
{
	struct sexp_bytevec *vec = sexp_bytevec(sexp);
	for (size_t i = 0; i < vec->size; i++)
		if (vec->data[i] != cstr[i])
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

static inline const char *strtype(enum sexp_type type)
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
	case SEXP_STRING:      return "string";
	case SEXP_SYMBOL:      return "symbol";
	case SEXP_VECTOR:      return "vector";
	case SEXP_BYTEVEC:     return "bytevector";
	case SEXP_MACRO:       return "macro";
	case SEXP_FUNCTION:    return "function";
	case SEXP_CASELAMBDA:  return "case-lambda";
	case SEXP_ESCAPE:      return "escape";
	case SEXP_ENVIRONMENT: return "environment";
	case SEXP_BOUNCE:      return "bounce";
	}
	return "unknown";
}

static inline sexp_t typesym(enum sexp_type type)
{
	return make_symbol(strtype(type));
}

/* type predicates */

#define TYPE_PREDICATE(name, type) \
	static inline bool name(sexp_t sexp) \
	{ \
		return sexp_type(sexp) == type; \
	}

TYPE_PREDICATE(is_void, SEXP_VOID)
TYPE_PREDICATE(is_nil,  SEXP_NIL)
TYPE_PREDICATE(is_eof,  SEXP_EOF)
TYPE_PREDICATE(is_num,  SEXP_NUM)
TYPE_PREDICATE(is_bool, SEXP_BOOL)
TYPE_PREDICATE(is_char, SEXP_CHAR)
TYPE_PREDICATE(is_values, SEXP_VALUES)
TYPE_PREDICATE(is_pair, SEXP_PAIR)
TYPE_PREDICATE(is_string, SEXP_STRING)
TYPE_PREDICATE(is_symbol, SEXP_SYMBOL)
TYPE_PREDICATE(is_vector, SEXP_VECTOR)
TYPE_PREDICATE(is_bytevec, SEXP_BYTEVEC)
TYPE_PREDICATE(is_macro, SEXP_MACRO)
TYPE_PREDICATE(is_function, SEXP_FUNCTION)
TYPE_PREDICATE(is_caselambda, SEXP_CASELAMBDA)
TYPE_PREDICATE(is_escape, SEXP_ESCAPE)
TYPE_PREDICATE(is_environment, SEXP_ENVIRONMENT)
TYPE_PREDICATE(is_bounce, SEXP_BOUNCE)

bool is_proper_list(sexp_t list);

#undef TYPE_PREDICATE
#endif
