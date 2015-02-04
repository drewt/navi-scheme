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

#include <string.h>

#include "navi.h"

#define SYMTAB_SIZE 64

struct navi_symbol {
	struct navi_hlist_node chain;
	struct navi_object object;
};

extern struct navi_clist_head active_environments;
static struct navi_hlist_head symbol_table[SYMTAB_SIZE];
static NAVI_LIST_HEAD(heap);

navi_t navi_sym_lambda;
navi_t navi_sym_caselambda;
navi_t navi_sym_define;
navi_t navi_sym_defmacro;
navi_t navi_sym_begin;
navi_t navi_sym_let;
navi_t navi_sym_seqlet;
navi_t navi_sym_letrec;
navi_t navi_sym_seqletrec;
navi_t navi_sym_set;
navi_t navi_sym_quote;
navi_t navi_sym_quasiquote;
navi_t navi_sym_unquote;
navi_t navi_sym_splice;
navi_t navi_sym_guard;
navi_t navi_sym_case;
navi_t navi_sym_cond;
navi_t navi_sym_if;
navi_t navi_sym_and;
navi_t navi_sym_or;
navi_t navi_sym_else;
navi_t navi_sym_eq_lt;
navi_t navi_sym_thunk;
navi_t navi_sym_question;
navi_t navi_sym_exn;
navi_t navi_sym_current_input;
navi_t navi_sym_current_output;
navi_t navi_sym_current_error;
navi_t navi_sym_read_error;
navi_t navi_sym_file_error;
navi_t navi_sym_repl;

void navi_free(struct navi_object *obj)
{
	obj->type = NAVI_VOID;
	free(obj);
}

static inline navi_t symbol_object(struct navi_symbol *sym)
{
	return (navi_t) &sym->object;
}

/* FIXME: this is a really bad hash function! */
static unsigned long symbol_hash(const char *symbol)
{
	unsigned long hash = 0;
	while (*symbol != '\0')
		hash += *symbol++;
	return hash;
}

static void symbol_table_init(void)
{
	for (unsigned i = 0; i < SYMTAB_SIZE; i++)
		NAVI_INIT_HLIST_HEAD(&symbol_table[i]);

	#define intern(cname, name) cname = navi_make_symbol(name)
	intern(navi_sym_lambda,         "lambda");
	intern(navi_sym_caselambda,     "case-lambda");
	intern(navi_sym_define,         "define");
	intern(navi_sym_defmacro,       "define-macro");
	intern(navi_sym_begin,          "begin");
	intern(navi_sym_let,            "let");
	intern(navi_sym_seqlet,         "let*");
	intern(navi_sym_letrec,         "letrec");
	intern(navi_sym_seqletrec,      "letrec*");
	intern(navi_sym_set,            "set!");
	intern(navi_sym_quote,          "quote");
	intern(navi_sym_quasiquote,     "quasiquote");
	intern(navi_sym_unquote,        "unquote");
	intern(navi_sym_splice,         "unquote-splice");
	intern(navi_sym_case,           "case");
	intern(navi_sym_cond,           "cond");
	intern(navi_sym_if,             "if");
	intern(navi_sym_and,            "and");
	intern(navi_sym_or,             "or");
	intern(navi_sym_else,           "else");
	intern(navi_sym_eq_lt,          "=>");
	intern(navi_sym_thunk,          "#thunk");
	intern(navi_sym_question,       "?");
	intern(navi_sym_exn,            "#exn");
	intern(navi_sym_current_input,  "#current-input-port");
	intern(navi_sym_current_output, "#current-output-port");
	intern(navi_sym_current_error,  "#current-error-port");
	intern(navi_sym_read_error,     "#read-error");
	intern(navi_sym_file_error,     "#file-error");
	intern(navi_sym_repl,           "#repl");
	#undef intern
}

void navi_init(void)
{
	symbol_table_init();
}

static navi_t symbol_lookup(const char *str, unsigned long hashcode)
{
	struct navi_symbol *it;
	struct navi_hlist_head *head = &symbol_table[hashcode % SYMTAB_SIZE];

	navi_hlist_for_each_entry(it, head, chain) {
		if (navi_bytevec_equal(symbol_object(it), str))
			return symbol_object(it);
	}
	return (navi_t) 0L;
}

/* Only called if symbol doesn't already exist */
static navi_t new_symbol(const char *str, unsigned long hashcode)
{
	size_t len = strlen(str);
	struct navi_symbol *symbol;
	
	symbol = navi_critical_malloc(sizeof(struct navi_symbol) +
			sizeof(struct navi_bytevec) + len + 1);

	for (size_t i = 0; i < len; i++)
		symbol->object.data->bvec.data[i] = str[i];
	symbol->object.data->bvec.data[len] = '\0';

	symbol->object.data->bvec.size = len;
	symbol->object.type = NAVI_SYMBOL;

	navi_hlist_add_head(&symbol->chain, &symbol_table[hashcode % SYMTAB_SIZE]);
	return (navi_t) &symbol->object;
}

DEFUN(scm_gensym, args, env)
{
	char buf[64];
	static unsigned count = 0;

	snprintf(buf, 64, "g%u", count++);
	buf[63] = '\0';
	return navi_make_uninterned(buf);
}

static struct navi_object *make_object(enum navi_type type, size_t size)
{
	struct navi_object *obj = navi_critical_malloc(sizeof(struct navi_object) + size);
	navi_clist_add(&obj->chain, &heap);
	obj->type = type;
	return obj;
}

navi_t navi_make_symbol(const char *str)
{
	unsigned long hashcode = symbol_hash(str);
	navi_t symbol = symbol_lookup(str, hashcode);

	if (symbol.n != 0)
		return symbol;
	return new_symbol(str, hashcode);
}

/* FIXME: unicode? */
navi_t navi_cstr_to_string(const char *str)
{
	size_t len = strlen(str);
	navi_t expr = navi_make_string(len, len, len);
	struct navi_string *vec = navi_string(expr);

	for (size_t i = 0; i < len; i++) {
		vec->data[i] = str[i];
	}
	vec->size = len;

	return expr;
}

navi_t navi_cstr_to_bytevec(const char *str)
{
	size_t len = strlen(str);
	navi_t expr = navi_make_bytevec(len);
	struct navi_bytevec *vec = navi_bytevec(expr);

	for (size_t i = 0; i < len; i++)
		vec->data[i] = str[i];
	vec->size = len;
	return expr;
}

navi_t navi_make_string(size_t storage, size_t size, size_t length)
{
	struct navi_object *str = make_object(NAVI_STRING, sizeof(struct navi_string));
	str->data->str.data = navi_critical_malloc(storage + 1);
	str->data->str.data[storage] = '\0';
	str->data->str.data[size] = '\0';
	str->data->str.storage = storage;
	str->data->str.size = size;
	str->data->str.length = length;
	return (navi_t) str;
}

navi_t navi_make_pair(navi_t car, navi_t cdr)
{
	struct navi_object *pair = make_object(NAVI_PAIR, sizeof(struct navi_pair));
	pair->data->pair.car = car;
	pair->data->pair.cdr = cdr;
	return (navi_t) pair;
}

navi_t navi_make_empty_pair(void)
{
	return (navi_t) make_object(NAVI_PAIR, sizeof(struct navi_pair));
}

navi_t navi_make_port(int(*read)(struct navi_port*, navi_env_t),
		void(*write)(unsigned char,struct navi_port*, navi_env_t),
		void(*close_in)(struct navi_port*, navi_env_t),
		void(*close_out)(struct navi_port*, navi_env_t),
		void *specific)
{
	struct navi_object *port = make_object(NAVI_PORT, sizeof(struct navi_port));
	port->data->port.read_u8 = read;
	port->data->port.write_u8 = write;
	port->data->port.close_in = close_in;
	port->data->port.close_out = close_out;
	port->data->port.flags = 0;
	port->data->port.expr = navi_make_void();
	port->data->port.pos = 0;
	port->data->port.specific = specific;
	return (navi_t) port;
}

navi_t navi_make_vector(size_t size)
{
	struct navi_object *vec = make_object(NAVI_VECTOR,
			sizeof(struct navi_vector) + sizeof(navi_t)*size);
	vec->data->vec.size = size;
	return (navi_t) vec;
}

navi_t navi_make_bytevec(size_t size)
{
	struct navi_object *vec = make_object(NAVI_BYTEVEC,
			sizeof(struct navi_bytevec) + size + 1);
	vec->data->bvec.size = size;
	vec->data->bvec.data[size] = '\0';
	return (navi_t) vec;
}

static inline unsigned count_pairs(navi_t list)
{
	navi_t cons;
	unsigned i = 0;
	navi_list_for_each(cons, list) { i++; }
	return i;
}

navi_t navi_make_function(navi_t args, navi_t body, char *name, navi_env_t env)
{
	struct navi_object *obj = make_object(NAVI_FUNCTION,
			sizeof(struct navi_function));
	struct navi_function *fun = &obj->data->fun;
	fun->name = name;
	fun->args = args;
	fun->body = body;
	fun->builtin = false;
	fun->arity = count_pairs((navi_t)args);
	fun->variadic = !navi_is_proper_list((navi_t)args);
	fun->env = env;
	navi_scope_ref(env);
	return (navi_t) obj;
}

navi_t navi_make_escape(void)
{
	return (navi_t) make_object(NAVI_ESCAPE, sizeof(struct navi_escape));
}

navi_t navi_capture_env(navi_env_t env)
{
	struct navi_object *obj = make_object(NAVI_ENVIRONMENT,
			sizeof(struct navi_scope*));
	obj->data->env = env;
	return (navi_t) obj;
}

navi_t navi_from_spec(struct navi_spec *spec)
{
	struct navi_object *obj;
	struct navi_function *fun;

	switch (spec->type) {
	case NAVI_EOF:
		return navi_make_eof();
	case NAVI_NIL:
		return navi_make_nil();
	case NAVI_NUM:
		return navi_make_num(spec->num);
	case NAVI_BOOL:
		return navi_make_bool(spec->num);
	case NAVI_CHAR:
		return navi_make_char(spec->num);
	case NAVI_PAIR:
		return navi_make_pair(spec->pair.car, spec->pair.cdr);
	case NAVI_STRING:
		return navi_cstr_to_string(spec->str);
	case NAVI_SYMBOL:
		return navi_make_symbol(spec->str);
	case NAVI_VECTOR:
		return navi_make_vector(spec->size);
	case NAVI_BYTEVEC:
		return navi_make_bytevec(spec->size);
	case NAVI_MACRO:
	case NAVI_SPECIAL:
	case NAVI_FUNCTION:
		obj = make_object(spec->type, sizeof(struct navi_function));
		fun = (void*) obj->data;
		memcpy(fun, &spec->fun, sizeof(struct navi_function));
		return (navi_t) obj;
	case NAVI_VOID:
	case NAVI_PORT:
	case NAVI_VALUES:
	case NAVI_PROMISE:
	case NAVI_CASELAMBDA:
	case NAVI_ESCAPE:
	case NAVI_ENVIRONMENT:
	case NAVI_BOUNCE:
		break;
	}
	navi_die("navi_from_spec: unknown or unsupported type");
}

bool navi_eqvp(navi_t fst, navi_t snd)
{
	if (navi_type(fst) != navi_type(snd))
		return false;

	switch (navi_type(fst)) {
	case NAVI_VOID:
	case NAVI_NIL:
	case NAVI_EOF:
		return true;
	case NAVI_NUM:
		return navi_num(fst) == navi_num(snd);
	case NAVI_BOOL:
		return navi_bool(fst) ? navi_bool(snd) : !navi_bool(snd);
	case NAVI_CHAR:
		return navi_char(fst) == navi_char(snd);
	case NAVI_PAIR:
	case NAVI_PORT:
	case NAVI_SYMBOL:
	case NAVI_VECTOR:
	case NAVI_VALUES:
	case NAVI_BYTEVEC:
	case NAVI_MACRO:
	case NAVI_SPECIAL:
	case NAVI_PROMISE:
	case NAVI_FUNCTION:
	case NAVI_CASELAMBDA:
	case NAVI_ESCAPE:
	case NAVI_ENVIRONMENT:
	case NAVI_BOUNCE:
		return fst.p == snd.p;
	case NAVI_STRING:
		return navi_string_equal(fst, snd);
	}
	navi_die("navi_eqvp: unknown type");
}

DEFUN(scm_eqvp, args, env)
{
	return navi_make_bool(navi_eqvp(navi_car(args), navi_cadr(args)));
}

static inline void gc_set_mark(navi_t obj)
{
	navi_ptr(obj)->gc_mark = true;
}

static void gc_mark_obj(navi_t obj)
{
	struct navi_vector *vec;
	struct navi_function *fun;

	switch (navi_type(obj)) {
	case NAVI_VOID:
	case NAVI_NIL:
	case NAVI_EOF:
	case NAVI_NUM:
	case NAVI_BOOL:
	case NAVI_CHAR:
		break;
	case NAVI_PAIR:
	case NAVI_BOUNCE:
		gc_set_mark(obj);
		gc_mark_obj(navi_car(obj));
		gc_mark_obj(navi_cdr(obj));
		break;
	case NAVI_PORT:
		gc_set_mark(obj);
		gc_mark_obj(navi_port(obj)->expr);
		break;
	case NAVI_SYMBOL:
	case NAVI_STRING:
	case NAVI_BYTEVEC:
		gc_set_mark(obj);
		break;
	case NAVI_VECTOR:
	case NAVI_VALUES:
	case NAVI_CASELAMBDA:
		gc_set_mark(obj);
		vec = navi_vector(obj);
		for (size_t i = 0; i < vec->size; i++)
			gc_mark_obj(vec->data[i]);
		break;
	case NAVI_MACRO:
	case NAVI_SPECIAL:
	case NAVI_PROMISE:
	case NAVI_FUNCTION:
		gc_set_mark(obj);
		fun = navi_fun(obj);
		if (!fun->builtin)
			gc_mark_obj(fun->body);
		gc_mark_obj(fun->args);
		break;
	case NAVI_ESCAPE:
		gc_set_mark(obj);
		gc_mark_obj(navi_escape(obj)->arg);
		break;
	case NAVI_ENVIRONMENT:
		gc_set_mark(obj);
		break;
	}
}

static void gc_mark_env(struct navi_scope *env)
{
	for (unsigned i = 0; i < NAVI_ENV_HT_SIZE; i++) {
		struct navi_binding *bind;
		navi_hlist_for_each_entry(bind, &env->bindings[i], chain) {
			if (navi_ptr_type(bind->object))
				gc_mark_obj(bind->object);
		}
	}
}

static void gc_mark(void)
{
	struct navi_scope *scope;
	navi_clist_for_each_entry(scope, &active_environments, chain) {
		gc_mark_env(scope);
	}
}

static void gc_sweep(void)
{
	struct navi_object *obj, *p;

	navi_clist_for_each_entry_safe(obj, p, &heap, chain) {
		if (obj->gc_mark) {
			obj->gc_mark = false;
			continue;
		}
		navi_clist_del(&obj->chain);
		navi_free(obj);
	}
}

void navi_gc_collect(void)
{
	gc_mark();
	gc_sweep();
}

DEFUN(scm_gc_collect, args, env)
{
	navi_gc_collect();
	return navi_unspecified();
}

DEFUN(scm_gc_count, args, env)
{
	struct navi_object *expr;
	navi_clist_for_each_entry(expr, &heap, chain) {
		if (navi_type((navi_t)expr) == NAVI_FUNCTION &&
				navi_fun((navi_t)expr)->builtin)
			continue;
		printf("<%p> ", expr);
		navi_write((navi_t)expr, env);
		putchar('\n');
	}
	return navi_unspecified();
}
