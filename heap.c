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

#include <unicode/ustring.h>

#include "navi.h"

#define SYMTAB_SIZE 64

extern struct navi_clist_head active_environments;
static struct navi_hlist_head symbol_table[SYMTAB_SIZE];
static NAVI_LIST_HEAD(heap);

navi_obj navi_sym_begin;
navi_obj navi_sym_quote;
navi_obj navi_sym_quasiquote;
navi_obj navi_sym_unquote;
navi_obj navi_sym_splice;
navi_obj navi_sym_else;
navi_obj navi_sym_eq_lt;
navi_obj navi_sym_exn;
navi_obj navi_sym_current_input;
navi_obj navi_sym_current_output;
navi_obj navi_sym_current_error;
navi_obj navi_sym_read_error;
navi_obj navi_sym_file_error;
navi_obj navi_sym_repl;

void navi_free(struct navi_object *obj)
{
	obj->type = NAVI_VOID;
	free(obj);
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
	intern(navi_sym_begin,          "begin");
	intern(navi_sym_quote,          "quote");
	intern(navi_sym_quasiquote,     "quasiquote");
	intern(navi_sym_unquote,        "unquote");
	intern(navi_sym_splice,         "unquote-splice");
	intern(navi_sym_else,           "else");
	intern(navi_sym_eq_lt,          "=>");
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

static navi_obj symbol_lookup(const char *str, unsigned long hashcode)
{
	struct navi_symbol *it;
	struct navi_hlist_head *head = &symbol_table[hashcode % SYMTAB_SIZE];

	navi_hlist_for_each_entry(it, head, chain) {
		if (!strcmp(it->data, str))
			return (navi_obj) navi_object(it);
	}
	return (navi_obj) 0L;
}

static struct navi_object *make_object(enum navi_type type, size_t size)
{
	struct navi_object *obj = navi_critical_malloc(sizeof(struct navi_object) + size);
	navi_clist_add(&obj->chain, &heap);
	obj->type = type;
	return obj;
}

navi_obj navi_make_uninterned(const char *str)
{
	size_t len = strlen(str);
	struct navi_object *object = make_object(NAVI_SYMBOL,
			sizeof(struct navi_symbol) + len + 1);
	struct navi_symbol *symbol = navi_symbol((navi_obj)object);

	for (size_t i = 0; i < len+1; i++)
		symbol->data[i] = str[i];
	return (navi_obj) object;
}

DEFUN(gensym, args, env, "gensym", 0, 0)
{
	char buf[64];
	static unsigned count = 0;

	snprintf(buf, 64, "g%u", count++);
	buf[63] = '\0';
	return navi_make_uninterned(buf);
}

/* Only called if symbol doesn't already exist */
static navi_obj new_symbol(const char *str, unsigned long hashcode)
{
	navi_obj object = navi_make_uninterned(str);
	struct navi_symbol *symbol = navi_symbol(object);
	navi_hlist_add_head(&symbol->chain, &symbol_table[hashcode % SYMTAB_SIZE]);
	return object;
}

navi_obj navi_make_symbol(const char *str)
{
	unsigned long hashcode = symbol_hash(str);
	navi_obj symbol = symbol_lookup(str, hashcode);

	if (symbol.n != 0)
		return symbol;
	return new_symbol(str, hashcode);
}

navi_obj navi_cstr_to_string(const char *cstr)
{
	size_t len = strlen(cstr);
	navi_obj obj = navi_make_string(len, len, len);
	struct navi_string *str = navi_string(obj);
	for (size_t i = 0; i < len; i++)
		str->data[i] = cstr[i];
	str->size = len;
	return obj;
}

navi_obj navi_cstr_to_bytevec(const char *str)
{
	size_t len = strlen(str);
	navi_obj expr = navi_make_bytevec(len);
	struct navi_bytevec *vec = navi_bytevec(expr);

	for (size_t i = 0; i < len; i++)
		vec->data[i] = str[i];
	vec->size = len;
	return expr;
}

navi_obj navi_make_string(size_t capacity, size_t size, size_t length)
{
	struct navi_object *str = make_object(NAVI_STRING, sizeof(struct navi_string));
	str->data->str.data = navi_critical_malloc(capacity + 1);
	str->data->str.data[capacity] = '\0';
	str->data->str.data[size] = '\0';
	str->data->str.capacity = capacity;
	str->data->str.size = size;
	str->data->str.length = length;
	return (navi_obj) str;
}

void navi_string_grow_storage(struct navi_string *str, long need)
{
	long free_space = str->capacity - str->size;
	if (free_space > need)
		return;
	need -= free_space;
	// TODO: align?
	str->data = navi_critical_realloc(str->data, str->capacity + need + 1);
	str->capacity += need;
	str->data[str->capacity] = '\0';
}

navi_obj navi_make_pair(navi_obj car, navi_obj cdr)
{
	struct navi_object *pair = make_object(NAVI_PAIR, sizeof(struct navi_pair));
	pair->data->pair.car = car;
	pair->data->pair.cdr = cdr;
	return (navi_obj) pair;
}

navi_obj navi_make_empty_pair(void)
{
	return (navi_obj) make_object(NAVI_PAIR, sizeof(struct navi_pair));
}

navi_obj navi_make_port(
		int(*read_u8)(struct navi_port*, navi_env),
		void(*write_u8)(uint8_t,struct navi_port*, navi_env),
		int32_t(*read_char)(struct navi_port*, navi_env),
		void(*write_char)(int32_t, struct navi_port*, navi_env),
		void(*close_in)(struct navi_port*, navi_env),
		void(*close_out)(struct navi_port*, navi_env),
		void *specific)
{
	struct navi_object *port = make_object(NAVI_PORT, sizeof(struct navi_port));
	port->data->port.read_u8 = read_u8;
	port->data->port.write_u8 = write_u8;
	port->data->port.read_char = read_char;
	port->data->port.write_char = write_char;
	port->data->port.close_in = close_in;
	port->data->port.close_out = close_out;
	port->data->port.flags = 0;
	port->data->port.expr = navi_make_void();
	port->data->port.pos = 0;
	port->data->port.specific = specific;
	return (navi_obj) port;
}

navi_obj navi_make_vector(size_t size)
{
	struct navi_object *vec = make_object(NAVI_VECTOR,
			sizeof(struct navi_vector) + sizeof(navi_obj)*size);
	vec->data->vec.size = size;
	return (navi_obj) vec;
}

navi_obj navi_make_bytevec(size_t size)
{
	struct navi_object *vec = make_object(NAVI_BYTEVEC,
			sizeof(struct navi_bytevec) + size + 1);
	vec->data->bvec.size = size;
	vec->data->bvec.data[size] = '\0';
	return (navi_obj) vec;
}

static inline unsigned count_pairs(navi_obj list)
{
	navi_obj cons;
	unsigned i = 0;
	navi_list_for_each(cons, list) { i++; }
	return i;
}

navi_obj navi_make_procedure(navi_obj args, navi_obj body, navi_obj name, navi_env env)
{
	struct navi_object *obj = make_object(NAVI_PROCEDURE,
			sizeof(struct navi_procedure));
	struct navi_procedure *proc = &obj->data->proc;
	proc->name = name;
	proc->args = args;
	proc->body = body;
	proc->env = env;
	proc->arity = count_pairs((navi_obj)args);
	proc->flags = 0;
	proc->types = NULL;
	if (!navi_is_proper_list(args))
		proc->flags |= NAVI_PROC_VARIADIC;
	navi_scope_ref(env);
	return (navi_obj) obj;
}

navi_obj navi_make_lambda(navi_obj args, navi_obj body, navi_env env)
{
	char buf[64];
	static unsigned long count = 0;
	snprintf(buf, 64, "lam%lu", count++);
	buf[63] = '\0';
	return navi_make_procedure(args, body, navi_make_uninterned(buf), env);
}

navi_obj navi_make_escape(void)
{
	return (navi_obj) make_object(NAVI_ESCAPE, sizeof(struct navi_escape));
}

navi_obj navi_capture_env(navi_env env)
{
	struct navi_object *obj = make_object(NAVI_ENVIRONMENT,
			sizeof(struct navi_scope*));
	obj->data->env = env;
	return (navi_obj) obj;
}

navi_obj navi_from_spec(struct navi_spec *spec)
{
	struct navi_object *obj;
	struct navi_procedure *proc;

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
	case NAVI_PROCEDURE:
		obj = make_object(spec->type, sizeof(struct navi_procedure));
		proc = (void*) obj->data;
		memcpy(proc, &spec->proc, sizeof(struct navi_procedure));
		proc->name = navi_make_symbol(spec->ident);
		return (navi_obj) obj;
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

bool navi_eqvp(navi_obj fst, navi_obj snd)
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
	case NAVI_PROCEDURE:
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

DEFUN(eqvp, args, env, "eqv?", 2, 0, NAVI_ANY, NAVI_ANY)
{
	return navi_make_bool(navi_eqvp(navi_car(args), navi_cadr(args)));
}

static inline void gc_set_mark(navi_obj obj)
{
	navi_ptr(obj)->gc_mark = true;
}

static void gc_mark_obj(navi_obj obj)
{
	struct navi_vector *vec;
	struct navi_procedure *proc;

	if (!navi_ptr_type(obj))
		return;
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
	case NAVI_PROCEDURE:
		gc_set_mark(obj);
		proc = navi_procedure(obj);
		if (!navi_proc_is_builtin(proc)) {
			gc_mark_obj(proc->body);
			gc_mark_obj(proc->args);
		}
		gc_set_mark(proc->name);
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
			gc_set_mark(bind->symbol);
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

DEFUN(gc_collect, args, env, "gc-collect", 0, 0)
{
	navi_gc_collect();
	return navi_unspecified();
}

DEFUN(gc_count, args, env, "gc-count", 0, 0)
{
	struct navi_object *expr;
	navi_clist_for_each_entry(expr, &heap, chain) {
		if (navi_is_builtin((navi_obj)expr))
			continue;
		printf("<%p> ", expr);
		navi_write((navi_obj)expr, env);
		putchar('\n');
	}
	return navi_unspecified();
}
