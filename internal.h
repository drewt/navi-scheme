/* Copyright 2014-2015 Drew Thoreson
 *
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/.
 */

#ifndef _NAVI_INTERNAL_H
#define _NAVI_INTERNAL_H

#include <setjmp.h>
#include <stdarg.h>
#include <stdbool.h>
#include <stddef.h>
#include <stdio.h>
#include <stdint.h>
#include <stdlib.h>
#include <string.h>

#include "navi.h"
#include "queue.h"

#define NAVI_ENV_HT_SIZE 64

/* C types {{{ */
struct navi_scope {
	NAVI_LIST_ENTRY(navi_scope) link;
	struct navi_scope *next;
	unsigned int refs;
	NAVI_LIST_HEAD(navi_bucket, navi_binding) bindings[NAVI_ENV_HT_SIZE];
};

typedef navi_obj (*navi_builtin)(unsigned, navi_obj, navi_env,
		struct navi_procedure*);
typedef navi_obj (*navi_leaf)(navi_obj, void*);

struct navi_escape {
	jmp_buf state;
	navi_env env;
	navi_obj arg;
};

enum {
	NAVI_PROC_BUILTIN  = 1,
	NAVI_PROC_VARIADIC = 2,
};

struct navi_thunk {
	navi_obj expr;
	navi_env env;
};

struct navi_procedure {
	uint32_t flags;
	unsigned arity;
	navi_obj name;
	struct navi_scope *env;
	union {
		struct {
			navi_obj args;
			navi_obj body;
		};
		navi_builtin c_proc;
	};
	navi_obj specific;
	const int *types;
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
	NAVI_LIST_ENTRY(navi_symbol) link;
	char data[];
};

struct navi_pair {
	navi_obj car;
	navi_obj cdr;
};

struct navi_port {
	int (*read_u8)(struct navi_port*, navi_env);
	void (*write_u8)(uint8_t, struct navi_port*, navi_env);
	long (*read_char)(struct navi_port*, navi_env);
	void (*write_char)(long, struct navi_port*, navi_env);
	void (*close_in)(struct navi_port*, navi_env);
	void (*close_out)(struct navi_port*, navi_env);
	unsigned long flags;
	int32_t buffer;
	navi_obj expr;
	int32_t pos;
	void *specific;
};

struct navi_spec {
	int type;
	union {
		void *ptr;
		int32_t num;
		const char *str;
		const struct navi_spec **elms;
		struct navi_procedure proc;
		struct navi_pair pair;
		struct {
			const struct navi_spec *param_value;
			const struct navi_spec *param_converter;
		};
	};
	size_t size;
	const char *ident;
	navi_obj (*init)(const struct navi_spec*);
};

struct navi_library {
	NAVI_LIST_ENTRY(navi_library) link;
	bool loaded;
	navi_obj name;
	union {
		navi_obj declarations;
		struct {
			navi_obj exports;
			navi_obj imports;
			navi_env env;
		};
	};
};

enum {
	NAVI_GC_MARK = 1,
	NAVI_PAT_ELLIPSIS = 2,
};

struct navi_object {
	NAVI_LIST_ENTRY(navi_object) link;
	enum navi_type type;
	uint16_t flags;
	_Alignas(sizeof(int)) unsigned char data[];
};

struct navi_binding {
	NAVI_LIST_ENTRY(navi_binding) link;
	navi_obj symbol;
	navi_obj object;
};
/* C types }}} */

#define navi_die(...) _navi_die(__FILE__, __LINE__, __VA_ARGS__)
_Noreturn int _navi_die(const char *file, int line, const char *msg, ...);
void *navi_critical_malloc(size_t size);
void *navi_critical_realloc(void *p, size_t size);

void navi_internal_init(void);

/* Memory Management {{{ */
void navi_free(struct navi_object *obj);
struct navi_scope *_navi_make_scope(void);
struct navi_scope *navi_make_scope(void);
void navi_scope_free(struct navi_scope *scope);

#undef _navi_scope_unref
static inline void _navi_scope_unref(struct navi_scope *scope)
{
	if (--scope->refs == 0)
		navi_scope_free(scope);
}

#undef _navi_scope_ref
static inline struct navi_scope *_navi_scope_ref(struct navi_scope *scope)
{
	scope->refs++;
	return scope;
}

#undef navi_env_ref
static inline void navi_env_ref(navi_env env)
{
	_navi_scope_ref(env.lexical);
	_navi_scope_ref(env.dynamic);
}

#undef navi_env_unref
static inline void navi_env_unref(navi_env env)
{
	_navi_scope_unref(env.lexical);
	_navi_scope_unref(env.dynamic);
}

#define navi_scope_for_each(binding, scope) \
	for (unsigned navi_i___ = 0; navi_i___ < NAVI_ENV_HT_SIZE; navi_i___++) \
		NAVI_LIST_FOREACH(binding, &scope->bindings[navi_i___], link)

#define navi_scope_for_each_safe(binding, n, scope) \
	for (unsigned navi_i___ = 0; navi_i___ < NAVI_ENV_HT_SIZE; navi_i___++) \
		NAVI_LIST_FOREACH_SAFE(binding, &scope->bindings[navi_i___], link, n)

/* Memory Management }}} */
/* Accessors {{{ */
#undef navi_ptr
static inline struct navi_object *navi_ptr(navi_obj obj)
{
	return obj.p;
}

#undef navi_vector
static inline struct navi_vector *navi_vector(navi_obj obj)
{
	return (struct navi_vector*) obj.p->data;
}

#undef navi_bytevec
static inline struct navi_bytevec *navi_bytevec(navi_obj obj)
{
	return (struct navi_bytevec*) obj.p->data;
}

#undef navi_string
static inline struct navi_string *navi_string(navi_obj obj)
{
	return (struct navi_string*) obj.p->data;
}

#undef navi_symbol
static inline struct navi_symbol *navi_symbol(navi_obj obj)
{
	return (struct navi_symbol*) obj.p->data;
}

#undef navi_thunk
static inline struct navi_thunk *navi_thunk(navi_obj obj)
{
	return (struct navi_thunk*) obj.p->data;
}

#undef navi_procedure
static inline struct navi_procedure *navi_procedure(navi_obj obj)
{
	return (struct navi_procedure*) obj.p->data;
}

#undef navi_escape
static inline struct navi_escape *navi_escape(navi_obj obj)
{
	return (struct navi_escape*) obj.p->data;
}

#undef navi_port
static inline struct navi_port *navi_port(navi_obj obj)
{
	return (struct navi_port*) obj.p->data;
}

#undef navi_environment
static inline navi_env navi_environment(navi_obj obj)
{
	// XXX: cast through a union to bypass the strict aliasing rule
	union { unsigned char *c; navi_env *e; } u = { .c = obj.p->data };
	return *(u.e);
}

#undef navi_pair
static inline struct navi_pair *navi_pair(navi_obj obj)
{
	return (struct navi_pair*) obj.p->data;
}

#define navi_container_of(ptr, type, member) \
	((type *)(void *)( (char *)(ptr) - offsetof(type, member) ))

#undef navi_object
static inline struct navi_object *navi_object(void *concrete)
{
	return navi_container_of(concrete, struct navi_object, data);
}

#undef navi_car
static inline navi_obj navi_car(navi_obj obj)
{
	return navi_pair(obj)->car;
}

#undef navi_cdr
static inline navi_obj navi_cdr(navi_obj obj)
{
	return navi_pair(obj)->cdr;
}

#undef navi_caar
static inline navi_obj navi_caar(navi_obj obj)
{
	return navi_pair(navi_car(obj))->car;
}

#undef navi_cadr
static inline navi_obj navi_cadr(navi_obj obj)
{
	return navi_pair(navi_cdr(obj))->car;
}

#undef navi_cdar
static inline navi_obj navi_cdar(navi_obj obj)
{
	return navi_pair(navi_car(obj))->cdr;
}

#undef navi_cddr
static inline navi_obj navi_cddr(navi_obj obj)
{
	return navi_pair(navi_cdr(obj))->cdr;
}

#undef navi_caddr
static inline navi_obj navi_caddr(navi_obj obj)
{
	return navi_pair(navi_cddr(obj))->car;
}

#undef navi_cadar
static inline navi_obj navi_cadar(navi_obj obj)
{
	return navi_pair(navi_cdar(obj))->car;
}

#undef navi_cdaar
static inline navi_obj navi_cdaar(navi_obj obj)
{
	return navi_pair(navi_caar(obj))->cdr;
}

#undef navi_cdddr
static inline navi_obj navi_cdddr(navi_obj obj)
{
	return navi_pair(navi_cddr(obj))->cdr;
}

#undef navi_cadddr
static inline navi_obj navi_cadddr(navi_obj obj)
{
	return navi_pair(navi_cdddr(obj))->car;
}

#undef navi_cddddr
static inline navi_obj navi_cddddr(navi_obj obj)
{
	return navi_pair(navi_cdddr(obj))->cdr;
}

#undef navi_caddddr
static inline navi_obj navi_caddddr(navi_obj obj)
{
	return navi_pair(navi_cddddr(obj))->car;
}

/* Accessors }}} */
/* Constructors {{{ */
navi_obj navi_from_spec(const struct navi_spec *spec, navi_env env);
void navi_string_grow_storage(struct navi_string *str, long need);
navi_obj navi_make_procedure(navi_obj args, navi_obj body, navi_obj name, navi_env env);
navi_obj navi_make_lambda(navi_obj args, navi_obj body, navi_env env);
navi_obj navi_make_thunk(navi_obj expr, navi_env env);
navi_obj navi_make_escape(void);
navi_obj _navi_make_parameter(navi_obj converter);
navi_obj navi_make_parameter(navi_obj value, navi_obj converter, navi_env env);
navi_obj _navi_make_named_parameter(navi_obj symbol, navi_obj converter);
navi_obj navi_make_named_parameter(navi_obj symbol, navi_obj value,
		navi_obj converter, navi_env env);

#undef navi_make_void
static inline navi_obj navi_make_void(void)
{
	return (navi_obj) { .n = NAVI_VOID_TAG };
}

#undef navi_make_nil
static inline navi_obj navi_make_nil(void)
{
	return (navi_obj) { .n = NAVI_NIL_TAG };
}

#undef navi_make_eof
static inline navi_obj navi_make_eof(void)
{
	return (navi_obj) { .n = NAVI_EOF_TAG };
}

#undef navi_make_fixnum
static inline navi_obj navi_make_fixnum(long num)
{
	return (navi_obj) { .n = (num << 1) | 1 };
}

#undef navi_make_bool
static inline navi_obj navi_make_bool(bool b)
{
	return (navi_obj) { .n = (b << NAVI_IMMEDIATE_TAG_BITS) | NAVI_BOOL_TAG };
}

#undef navi_make_char
static inline navi_obj navi_make_char(unsigned long c)
{
	return (navi_obj) { .n = (c << NAVI_IMMEDIATE_TAG_BITS) | NAVI_CHAR_TAG };
}

#undef navi_make_macro
static inline navi_obj navi_make_macro(navi_obj args, navi_obj body, navi_obj name,
		navi_env env)
{
	navi_obj macro = navi_make_procedure(args, body, name, env);
	macro.p->type = NAVI_MACRO;
	return macro;
}

#undef navi_make_promise
static inline navi_obj navi_make_promise(navi_obj e, navi_env env)
{
	navi_obj body = navi_make_pair(e, navi_make_nil());
	navi_obj promise = navi_make_procedure(navi_make_nil(), body,
			navi_make_symbol("promise"), env);
	promise.p->type = NAVI_PROMISE;
	return promise;
}

#undef navi_make_caselambda
static inline navi_obj navi_make_caselambda(size_t size)
{
	navi_obj lambda = navi_make_vector(size);
	lambda.p->type = NAVI_CASELAMBDA;
	return lambda;
}

#undef navi_make_apair
static inline navi_obj navi_make_apair(const char *sym, navi_obj val)
{
	return navi_make_pair(navi_make_symbol(sym), val);
}

#undef navi_make_bounce
static inline navi_obj navi_make_bounce(navi_obj object, navi_env env)
{
	navi_obj bounce = navi_make_thunk(object, env);
	bounce.p->type = NAVI_BOUNCE;
	return bounce;
}

#undef navi_unspecified
static inline navi_obj navi_unspecified(void)
{
	return navi_make_void();
}
/* Constructors }}} */
/* Environments/Evaluation {{{ */
navi_env navi_extend_environment(navi_env env, navi_obj vars, navi_obj args);

#undef navi_env_lookup
static inline navi_obj navi_env_lookup(struct navi_scope *env, navi_obj symbol)
{
	struct navi_binding *binding = navi_env_binding(env, symbol);
	return binding == NULL ? navi_make_void() : binding->object;
}

#undef navi_apply
static inline navi_obj navi_apply(struct navi_procedure *proc, navi_obj args,
		navi_env env)
{
	navi_env proc_env = {
		.lexical = proc->env,
		.dynamic = env.dynamic
	};
	return _navi_apply(proc, args, proc_env);
}
/* Environments/Evaluation }}} */
/* Types {{{ */
#undef navi_immediate_type
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

#undef navi_type
static inline enum navi_type navi_type(navi_obj obj)
{
	if (obj.n == 0)
		return NAVI_VOID;
	if (obj.n & 1)
		return NAVI_FIXNUM;
	if (obj.n & 2)
		return navi_immediate_type(obj);
	return obj.p->type;
}

#undef navi_ptr_type
static inline bool navi_ptr_type(navi_obj obj)
{
	return obj.n != 0 && (obj.n & 3) == 0;
}

#undef navi_strtype
static inline const char *navi_strtype(enum navi_type type)
{
	switch (type) {
	case NAVI_VOID:        return "void";
	case NAVI_NIL:         return "nil";
	case NAVI_EOF:         return "eof-object";
	case NAVI_FIXNUM:      return "number";
	case NAVI_BOOL:        return "boolean";
	case NAVI_CHAR:        return "character";
	case NAVI_VALUES:      return "values";
	case NAVI_PAIR:        return "pair";
	case NAVI_PORT:        return "port";
	case NAVI_STRING:      return "string";
	case NAVI_SYMBOL:      return "symbol";
	case NAVI_VECTOR:      return "vector";
	case NAVI_BYTEVEC:     return "bytevector";
	case NAVI_THUNK:       return "thunk";
	case NAVI_MACRO:       return "macro";
	case NAVI_SPECIAL:     return "special";
	case NAVI_PROMISE:     return "promise";
	case NAVI_PROCEDURE:   return "procedure";
	case NAVI_CASELAMBDA:  return "case-lambda";
	case NAVI_ESCAPE:      return "escape";
	case NAVI_PARAMETER:   return "parameter";
	case NAVI_ENVIRONMENT: return "environment";
	case NAVI_BOUNCE:      return "bounce";
	}
	return "unknown";
}

#undef navi_typesym
static inline navi_obj navi_typesym(enum navi_type type)
{
	return navi_make_symbol(navi_strtype(type));
}

#define NAVI_TYPE_PREDICATE(name, type) \
	static inline bool name(navi_obj obj) \
	{ \
		return navi_type(obj) == type; \
	}

#undef navi_is_void
NAVI_TYPE_PREDICATE(navi_is_void, NAVI_VOID)
#undef navi_is_nil
NAVI_TYPE_PREDICATE(navi_is_nil,  NAVI_NIL)
#undef navi_is_eof
NAVI_TYPE_PREDICATE(navi_is_eof,  NAVI_EOF)
#undef navi_is_fixnum
NAVI_TYPE_PREDICATE(navi_is_fixnum,  NAVI_FIXNUM)
#undef navi_is_bool
NAVI_TYPE_PREDICATE(navi_is_bool, NAVI_BOOL)
#undef navi_is_char
NAVI_TYPE_PREDICATE(navi_is_char, NAVI_CHAR)
#undef navi_is_values
NAVI_TYPE_PREDICATE(navi_is_values, NAVI_VALUES)
#undef navi_is_pair
NAVI_TYPE_PREDICATE(navi_is_pair, NAVI_PAIR)
#undef navi_is_port
NAVI_TYPE_PREDICATE(navi_is_port, NAVI_PORT)
#undef navi_is_string
NAVI_TYPE_PREDICATE(navi_is_string, NAVI_STRING)
#undef navi_is_symbol
NAVI_TYPE_PREDICATE(navi_is_symbol, NAVI_SYMBOL)
#undef navi_is_vector
NAVI_TYPE_PREDICATE(navi_is_vector, NAVI_VECTOR)
#undef navi_is_bytevec
NAVI_TYPE_PREDICATE(navi_is_bytevec, NAVI_BYTEVEC)
#undef navi_is_macro
NAVI_TYPE_PREDICATE(navi_is_macro, NAVI_MACRO)
#undef navi_is_procedure
NAVI_TYPE_PREDICATE(navi_is_procedure, NAVI_PROCEDURE)
#undef navi_is_caselambda
NAVI_TYPE_PREDICATE(navi_is_caselambda, NAVI_CASELAMBDA)
#undef navi_is_escape
NAVI_TYPE_PREDICATE(navi_is_escape, NAVI_ESCAPE)
#undef navi_is_parameter
NAVI_TYPE_PREDICATE(navi_is_parameter, NAVI_PARAMETER)
#undef navi_is_environment
NAVI_TYPE_PREDICATE(navi_is_environment, NAVI_ENVIRONMENT)
#undef navi_is_bounce
NAVI_TYPE_PREDICATE(navi_is_bounce, NAVI_BOUNCE)
#undef NAVI_TYPE_PREDICATE

#undef navi_is_byte
static inline bool navi_is_byte(navi_obj obj)
{
	return navi_is_fixnum(obj) && navi_fixnum(obj) >= 0 && navi_fixnum(obj) < 256;
}

#undef navi_is_builtin
static inline bool navi_is_builtin(navi_obj obj)
{
	return navi_is_procedure(obj) &&
		navi_procedure(obj)->flags & NAVI_PROC_BUILTIN;
}

#undef navi_is_list
static inline bool navi_is_list(navi_obj obj)
{
	return navi_type(obj) == NAVI_PAIR || navi_type(obj) == NAVI_NIL;
}
/* Types }}} */
/* Procedures {{{ */
#undef navi_proc_is_builtin
static inline bool navi_proc_is_builtin(struct navi_procedure *p)
{
	return p->flags & NAVI_PROC_BUILTIN;
}

#undef navi_proc_is_variadic
static inline bool navi_proc_is_variadic(struct navi_procedure *p)
{
	return p->flags & NAVI_PROC_VARIADIC;
}

#undef navi_arity_satisfied
static inline bool navi_arity_satisfied(struct navi_procedure *p, unsigned n)
{
	return n == p->arity
		|| ((p->flags & NAVI_PROC_VARIADIC) && n > p->arity);
}
/* Procedures }}} */
/* Pairs/Lists {{{ */
navi_obj navi_vlist(navi_obj first, va_list ap);
navi_obj _navi_list(navi_obj first, ...);
#define navi_list(...) _navi_list(__VA_ARGS__, navi_make_void())
int navi_list_length_safe(navi_obj list);
bool navi_is_list_of(navi_obj list, int type, bool allow_dotted_tail);
navi_obj navi_map(navi_obj list, navi_leaf fn, void *data);

#undef navi_set_car
static inline void navi_set_car(navi_obj cons, navi_obj obj)
{
	navi_pair(cons)->car = obj;
}

#undef navi_set_cdr
static inline void navi_set_cdr(navi_obj cons, navi_obj obj)
{
	navi_pair(cons)->cdr = obj;
}

#undef navi_last_cons
static inline navi_obj navi_last_cons(navi_obj list)
{
	while (navi_type(navi_cdr(list)) == NAVI_PAIR)
		list = navi_cdr(list);
	return list;
}

#undef navi_is_last_pair
static inline bool navi_is_last_pair(navi_obj pair)
{
	return navi_type(navi_cdr(pair)) == NAVI_NIL;
}
/* Lists }}} */
/* Ports {{{ */
#undef navi_display
static inline void navi_display(navi_obj obj, navi_env env)
{
	navi_port_display(navi_port(navi_current_output_port(env)), obj, env);
}

#undef navi_write
static inline void navi_write(navi_obj obj, navi_env env)
{
	navi_port_write(navi_port(navi_current_output_port(env)), obj, env);
}

#undef navi_port_is_input_port
static inline bool navi_port_is_input_port(struct navi_port *p)
{
	return p->read_u8 || p->read_char;
}

#undef navi_port_is_output_port
static inline bool navi_port_is_output_port(struct navi_port *p)
{
	return p->write_u8 || p->write_char;
}

#undef navi_is_input_port
static inline bool navi_is_input_port(navi_obj obj)
{
	return navi_is_port(obj) && navi_port_is_input_port(navi_port(obj));
}

#undef navi_is_output_port
static inline bool navi_is_output_port(navi_obj obj)
{
	return navi_is_port(obj) && navi_port_is_output_port(navi_port(obj));
}

/* Ports }}} */
/* Vectors {{{ */
navi_obj navi_vector_map(navi_obj proc, navi_obj to, navi_obj from, navi_env env);

#undef navi_vector_ref
static inline navi_obj navi_vector_ref(navi_obj vec, size_t i)
{
	return navi_vector(vec)->data[i];
}

#undef navi_vector_length
static inline size_t navi_vector_length(navi_obj vec)
{
	return navi_vector(vec)->size;
}
/* Vectors }}} */
/* Bytevectors {{{ */
bool navi_bytevec_equal(navi_obj obj, const char *cstr);

#undef navi_bytevec_ref
static inline navi_obj navi_bytevec_ref(navi_obj vec, size_t i)
{
	return navi_make_fixnum(navi_bytevec(vec)->data[i]);
}

#undef navi_bytevec_length
static inline size_t navi_bytevec_length(navi_obj vec)
{
	return navi_bytevec(vec)->size;
}
/* Bytevectors }}} */
/* Parameters {{{ */
#define navi_parameter_key(prm) navi_car(prm)
#define navi_parameter_converter(prm) navi_cdr(prm)
/* Parameters }}} */
/* Misc {{{ */
#undef navi_is_true
static inline bool navi_is_true(navi_obj expr)
{
	return navi_type(expr) != NAVI_BOOL || navi_bool(expr);
}

#undef navi_symbol_eq
static inline bool navi_symbol_eq(navi_obj expr, navi_obj symbol)
{
	return navi_is_symbol(expr) && expr.p == symbol.p;
}

#undef navi_force_tail
static inline navi_obj navi_force_tail(navi_obj obj, navi_env env)
{
	if (navi_type(obj) == NAVI_BOUNCE)
		return navi_eval(obj, env);
	return obj;
}
/* Misc }}} */

#include "error.h"
#include "lib.h"
#include "macros.h"
#include "symbols.h"

#endif
