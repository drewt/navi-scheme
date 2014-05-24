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

#include <string.h>

#include "sexp.h"

void sexp_free(struct sexp *sexp)
{
	sexp->type = SEXP_VOID;
	free(sexp);
}

#define SYMTAB_SIZE 64

struct sexp_symbol {
	struct hlist_node chain;
	struct sexp object;
};

struct hlist_head symbol_table[SYMTAB_SIZE];

LIST_HEAD(heap);

sexp_t sym_lambda;
sexp_t sym_caselambda;
sexp_t sym_define;
sexp_t sym_defmacro;
sexp_t sym_begin;
sexp_t sym_let;
sexp_t sym_seqlet;
sexp_t sym_letrec;
sexp_t sym_seqletrec;
sexp_t sym_set;
sexp_t sym_quote;
sexp_t sym_quasiquote;
sexp_t sym_unquote;
sexp_t sym_splice;
sexp_t sym_guard;
sexp_t sym_case;
sexp_t sym_cond;
sexp_t sym_if;
sexp_t sym_and;
sexp_t sym_or;

sexp_t sym_else;
sexp_t sym_eq_lt;

sexp_t sym_thunk;
sexp_t sym_question;

sexp_t sym_exn;
sexp_t sym_current_input;
sexp_t sym_current_output;
sexp_t sym_current_error;
sexp_t sym_repl;

static inline sexp_t symbol_object(struct sexp_symbol *sym)
{
	return (sexp_t) &sym->object;
}

/* FIXME: this is a really bad hash function! */
unsigned long symbol_hash(const char *symbol)
{
	unsigned long hash = 0;
	while (*symbol != '\0')
		hash += *symbol++;
	return hash;
}

void symbol_table_init(void)
{
	for (unsigned i = 0; i < SYMTAB_SIZE; i++)
		INIT_HLIST_HEAD(&symbol_table[i]);

	#define intern(cname, name) cname = make_symbol(name)
	intern(sym_lambda,         "lambda");
	intern(sym_caselambda,     "case-lambda");
	intern(sym_define,         "define");
	intern(sym_defmacro,       "define-macro");
	intern(sym_begin,          "begin");
	intern(sym_let,            "let");
	intern(sym_seqlet,         "let*");
	intern(sym_letrec,         "letrec");
	intern(sym_seqletrec,      "letrec*");
	intern(sym_set,            "set!");
	intern(sym_quote,          "quote");
	intern(sym_quasiquote,     "quasiquote");
	intern(sym_unquote,        "unquote");
	intern(sym_splice,         "unquote-splice");
	intern(sym_case,           "case");
	intern(sym_cond,           "cond");
	intern(sym_if,             "if");
	intern(sym_and,            "and");
	intern(sym_or,             "or");
	intern(sym_else,           "else");
	intern(sym_eq_lt,          "=>");
	intern(sym_thunk,          "#thunk");
	intern(sym_question,       "?");
	intern(sym_exn,            "#exn");
	intern(sym_current_input,  "#current-input-port");
	intern(sym_current_output, "#current-output-port");
	intern(sym_current_error,  "#current-error-port");
	intern(sym_repl,           "#repl");
	#undef intern
}

static sexp_t symbol_lookup(const char *str, unsigned long hashcode)
{
	struct sexp_symbol *it;
	struct hlist_head *head = &symbol_table[hashcode % SYMTAB_SIZE];

	hlist_for_each_entry(it, head, chain) {
		if (bytevec_equal(symbol_object(it), str))
			return symbol_object(it);
	}
	return (sexp_t) 0UL;
}

/* Only called if symbol doesn't already exist */
static sexp_t new_symbol(const char *str, unsigned long hashcode)
{
	size_t len = strlen(str);
	struct sexp_symbol *symbol;
	
	symbol = malloc(sizeof(struct sexp_symbol) +
			sizeof(struct sexp_bytevec) + len + 1);

	for (size_t i = 0; i < len; i++)
		symbol->object.data->bvec.data[i] = str[i];
	symbol->object.data->bvec.data[len] = '\0';

	symbol->object.data->bvec.size = len;
	symbol->object.type = SEXP_SYMBOL;

	hlist_add_head(&symbol->chain, &symbol_table[hashcode % SYMTAB_SIZE]);
	return (sexp_t) &symbol->object;
}

DEFUN(scm_gensym, args)
{
	char buf[64];
	static unsigned count = 0;

	snprintf(buf, 64, "g%u", count++);
	buf[63] = '\0';
	return make_uninterned(buf);
}

struct sexp *make_sexp(enum sexp_type type, size_t size)
{
	struct sexp *sexp = malloc(sizeof(struct sexp) + size);
	list_add(&sexp->chain, &heap);
	sexp->type = type;
	return sexp;
}

sexp_t make_symbol(const char *str)
{
	unsigned long hashcode = symbol_hash(str);
	sexp_t symbol = symbol_lookup(str, hashcode);

	if (symbol.n != 0)
		return symbol;
	return new_symbol(str, hashcode);
}

sexp_t to_string(const char *str)
{
	size_t len = strlen(str);
	sexp_t sexp = make_string(len);
	struct sexp_string *vec = sexp_string(sexp);

	for (size_t i = 0; i < len; i++) {
		vec->data[i] = str[i];
	}
	vec->size = len;

	return sexp;
}

sexp_t to_bytevec(const char *str)
{
	size_t len = strlen(str);
	sexp_t sexp = make_bytevec(len);
	struct sexp_bytevec *vec = sexp_bytevec(sexp);

	for (size_t i = 0; i < len; i++)
		vec->data[i] = str[i];
	vec->size = len;
	return sexp;
}

sexp_t make_string(size_t size)
{
	struct sexp *sexp = make_sexp(SEXP_STRING, sizeof(struct sexp_string));
	sexp->data->str.data = malloc(size + 1);
	sexp->data->str.data[size] = '\0';
	sexp->data->str.size = size + 1;
	sexp->data->str.length = 0;
	return (sexp_t) sexp;
}

sexp_t make_pair(sexp_t car, sexp_t cdr)
{
	struct sexp *sexp = make_sexp(SEXP_PAIR, sizeof(struct sexp_pair));
	sexp->data->pair.car = car;
	sexp->data->pair.cdr = cdr;
	return (sexp_t) sexp;
}

sexp_t make_empty_pair(void)
{
	return (sexp_t) make_sexp(SEXP_PAIR, sizeof(struct sexp_pair));
}

sexp_t make_port(sexp_t(*read)(struct sexp_port*),
		void(*write)(sexp_t,struct sexp_port*), void *specific)
{
	struct sexp *sexp = make_sexp(SEXP_PORT, sizeof(struct sexp_port));
	sexp->data->port.read_u8 = read;
	sexp->data->port.write_u8 = write;
	sexp->data->port.buffer_full = false;
	sexp->data->port.eof = false;
	sexp->data->port.sexp = make_void();
	sexp->data->port.pos = 0;
	sexp->data->port.specific = specific;
	return (sexp_t) sexp;
}

sexp_t make_vector(size_t size)
{
	struct sexp *sexp = make_sexp(SEXP_VECTOR,
			sizeof(struct sexp_vector) + sizeof(sexp_t)*size);
	sexp->data->vec.size = size;
	return (sexp_t) sexp;
}

sexp_t make_bytevec(size_t size)
{
	struct sexp *sexp = make_sexp(SEXP_BYTEVEC,
			sizeof(struct sexp_bytevec) + size + 1);
	sexp->data->bvec.size = size;
	sexp->data->bvec.data[size] = '\0';
	return (sexp_t) sexp;
}

sexp_t make_function(sexp_t args, sexp_t body, char *name, env_t env)
{
	struct sexp *sexp = make_sexp(SEXP_FUNCTION,
			sizeof(struct sexp_function));
	struct sexp_function *fun = &sexp->data->fun;
	fun->name = name;
	fun->args = args;
	fun->body = body;
	fun->builtin = false;
	fun->arity = count_pairs((sexp_t)args);
	fun->variadic = !list_is_proper((sexp_t)args);
	fun->env = env;
	scope_ref(env);
	return (sexp_t) sexp;
}

sexp_t make_thunk(sexp_t body, env_t env)
{
	return make_function(make_nil(), body, "#thunk", env);
}

sexp_t make_escape(void)
{
	return (sexp_t) make_sexp(SEXP_ESCAPE, sizeof(struct sexp_escape));
}

sexp_t capture_env(env_t env)
{
	struct sexp *sexp = make_sexp(SEXP_ENVIRONMENT,
			sizeof(struct sexp_scope*));
	sexp->data->env = env;
	return (sexp_t) sexp;
}

sexp_t sexp_from_spec(struct sexp_spec *spec)
{
	struct sexp *sexp;
	struct sexp_function *fun;

	switch (spec->type) {
	case SEXP_EOF:
		return make_eof();
	case SEXP_NIL:
		return make_nil();
	case SEXP_NUM:
		return make_num(spec->num);
	case SEXP_BOOL:
		return make_bool(spec->num);
	case SEXP_CHAR:
		return make_char(spec->num);
	case SEXP_PAIR:
		return make_pair(spec->pair.car, spec->pair.cdr);
	case SEXP_STRING:
		return to_string(spec->str);
	case SEXP_SYMBOL:
		return make_symbol(spec->str);
	case SEXP_VECTOR:
		return make_vector(spec->size);
	case SEXP_BYTEVEC:
		return make_bytevec(spec->size);
	case SEXP_MACRO:
	case SEXP_FUNCTION:
		sexp = make_sexp(spec->type, sizeof(struct sexp_function));
		fun = (void*) sexp->data;
		memcpy(fun, &spec->fun, sizeof(struct sexp_function));
		return (sexp_t) sexp;
	case SEXP_VOID:
	case SEXP_PORT:
	case SEXP_VALUES:
	case SEXP_CASELAMBDA:
	case SEXP_ESCAPE:
	case SEXP_ENVIRONMENT:
	case SEXP_BOUNCE:
		break;
	}
	die("sexp_from_spec: unknown or unsupported type");
}

bool eqvp(sexp_t fst, sexp_t snd)
{
	if (sexp_type(fst) != sexp_type(snd))
		return false;

	switch (sexp_type(fst)) {
	case SEXP_VOID:
	case SEXP_NIL:
	case SEXP_EOF:
		return true;
	case SEXP_NUM:
		return sexp_num(fst) == sexp_num(snd);
	case SEXP_BOOL:
		return sexp_bool(fst) ? sexp_bool(snd) : !sexp_bool(snd);
	case SEXP_CHAR:
		return sexp_char(fst) == sexp_char(snd);
	case SEXP_PAIR:
	case SEXP_PORT:
	case SEXP_SYMBOL:
	case SEXP_VECTOR:
	case SEXP_VALUES:
	case SEXP_BYTEVEC:
	case SEXP_MACRO:
	case SEXP_FUNCTION:
	case SEXP_CASELAMBDA:
	case SEXP_ESCAPE:
	case SEXP_ENVIRONMENT:
	case SEXP_BOUNCE:
		return fst.p == snd.p;
	case SEXP_STRING:
		return sexp_string_equal(fst, snd);
	}
	die("unknown type");
}

DEFUN(scm_eqvp, args)
{
	return make_bool(eqvp(car(args), cadr(args)));
}

static inline void gc_set_mark(sexp_t obj)
{
	sexp_ptr(obj)->gc_mark = true;
}

static void gc_mark_obj(sexp_t obj)
{
	struct sexp_vector *vec;
	struct sexp_function *fun;

	switch (sexp_type(obj)) {
	case SEXP_VOID:
	case SEXP_NIL:
	case SEXP_EOF:
	case SEXP_NUM:
	case SEXP_BOOL:
	case SEXP_CHAR:
		break;
	case SEXP_PAIR:
	case SEXP_BOUNCE:
		gc_set_mark(obj);
		gc_mark_obj(car(obj));
		gc_mark_obj(cdr(obj));
		break;
	case SEXP_PORT:
		gc_set_mark(obj);
		gc_mark_obj(sexp_port(obj)->sexp);
		break;
	case SEXP_SYMBOL:
	case SEXP_STRING:
	case SEXP_BYTEVEC:
		gc_set_mark(obj);
		break;
	case SEXP_VECTOR:
	case SEXP_VALUES:
	case SEXP_CASELAMBDA:
		gc_set_mark(obj);
		vec = sexp_vector(obj);
		for (size_t i = 0; i < vec->size; i++)
			gc_mark_obj(vec->data[i]);
		break;
	case SEXP_MACRO:
	case SEXP_FUNCTION:
		gc_set_mark(obj);
		fun = sexp_fun(obj);
		if (!fun->builtin)
			gc_mark_obj(fun->body);
		gc_mark_obj(fun->args);
		break;
	case SEXP_ESCAPE:
		gc_set_mark(obj);
		gc_mark_obj(sexp_escape(obj)->arg);
		break;
	case SEXP_ENVIRONMENT:
		gc_set_mark(obj);
		break;
	}
}

static void gc_mark_env(struct sexp_scope *env)
{
	for (unsigned i = 0; i < ENV_HT_SIZE; i++) {
		struct sexp_binding *bind;
		hlist_for_each_entry(bind, &env->bindings[i], chain) {
			if (ptr_type(bind->object))
				gc_mark_obj(bind->object);
		}
	}
}

static void gc_mark(void)
{
	struct sexp_scope *scope;
	list_for_each_entry(scope, &active_environments, chain) {
		gc_mark_env(scope);
	}
}

static void gc_sweep(void)
{
	struct sexp *sexp, *p;

	list_for_each_entry_safe(sexp, p, &heap, chain) {
		if (sexp->gc_mark) {
			sexp->gc_mark = false;
			continue;
		}
		list_del(&sexp->chain);
		sexp_free(sexp);
	}
}

void invoke_gc(void)
{
	gc_mark();
	gc_sweep();
}

DEFUN(scm_gc_collect, args)
{
	invoke_gc();
	return unspecified();
}

DEFUN(scm_gc_count, args)
{
	struct sexp *sexp;
	list_for_each_entry(sexp, &heap, chain) {
		if (sexp_type((sexp_t)sexp) == SEXP_FUNCTION &&
				sexp_fun((sexp_t)sexp)->builtin)
			continue;
		printf("<%p> ", sexp);
		sexp_write((sexp_t)sexp, ____env);
		putchar('\n');
	}
	return unspecified();
}
