/* Copyright 2014-2015 Drew Thoreson
 *
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/.
 */

#include "slab.h"

#define SYMTAB_SIZE 64

extern NAVI_LIST_HEAD(active_environments, navi_scope) active_environments;
static NAVI_SLIST_HEAD(heap, navi_object) heap = NAVI_SLIST_HEAD_INITIALIZER(heap);
static NAVI_LIST_HEAD(sym_bucket, navi_symbol) symbol_table[SYMTAB_SIZE];

static struct slab_cache *pair_cache = NULL;
static struct slab_cache *guard_cache = NULL;
static struct slab_cache *binding_cache = NULL;

navi_obj navi_sym_begin;
navi_obj navi_sym_quote;
navi_obj navi_sym_quasiquote;
navi_obj navi_sym_unquote;
navi_obj navi_sym_splice;
navi_obj navi_sym_else;
navi_obj navi_sym_eq_lt;
navi_obj navi_sym_lib_paths;
navi_obj navi_sym_command_line;
navi_obj navi_sym_current_exn;
navi_obj navi_sym_current_input;
navi_obj navi_sym_current_output;
navi_obj navi_sym_current_error;
navi_obj navi_sym_read_error;
navi_obj navi_sym_file_error;
navi_obj navi_sym_internal_error;
navi_obj navi_sym_repl;
navi_obj navi_sym_export;
navi_obj navi_sym_import;
navi_obj navi_sym_only;
navi_obj navi_sym_except;
navi_obj navi_sym_prefix;
navi_obj navi_sym_rename;
navi_obj navi_sym_deflib;
navi_obj navi_sym_include;
navi_obj navi_sym_include_ci;
navi_obj navi_sym_include_libdecl;
navi_obj navi_sym_cond_expand;
navi_obj navi_sym_ellipsis;
navi_obj navi_sym_underscore;

static struct {
	size_t bytes;
	size_t objects;
	size_t threshold;
} gc_stats = {0};

void *navi_critical_malloc(size_t size)
{
	void *r;
	if (!(r = malloc(size)))
		navi_die("not enough memory");
	return r;
}

void *navi_critical_realloc(void *p, size_t size)
{
	void *r;
	if (!(r = realloc(p, size)))
		navi_die("not enough memory");
	return r;
}

static navi_obj to_obj(struct navi_object *obj)
{
	return (navi_obj) { .p = obj };
}

static __const size_t object_size(struct navi_object *obj)
{
	switch(obj->type) {
	case NAVI_VOID: case NAVI_NIL:  case NAVI_FIXNUM:
	case NAVI_EOF:  case NAVI_BOOL: case NAVI_CHAR:
		return 0;
	case NAVI_PAIR:
	case NAVI_PARAMETER:
		return sizeof(struct navi_pair);
	case NAVI_PORT:
		return sizeof(struct navi_port);
	case NAVI_STRING:
		return navi_string(to_obj(obj))->capacity
			+ sizeof(struct navi_string);
	case NAVI_SYMBOL:
		return strlen(navi_symbol(to_obj(obj))->data)
			+ sizeof(struct navi_symbol);
	case NAVI_VECTOR:
	case NAVI_VALUES:
	case NAVI_CASELAMBDA:
		return navi_vector(to_obj(obj))->size * sizeof(navi_obj)
			+ sizeof(struct navi_vector);
	case NAVI_BYTEVEC:
		return navi_bytevec(to_obj(obj))->size
			+ sizeof(struct navi_bytevec);
	case NAVI_MACRO:
	case NAVI_SPECIAL:
	case NAVI_PROMISE:
	case NAVI_PROCEDURE:
		return sizeof(struct navi_procedure);
	case NAVI_THUNK:
	case NAVI_BOUNCE:
		return sizeof(struct navi_thunk);
	case NAVI_ESCAPE:
		return sizeof(struct navi_escape);
	case NAVI_ENVIRONMENT:
		return sizeof(navi_env);
	case NAVI_TRAP:
		navi_die("trap!");
	}
	return 0;
}

/*void dprint(navi_obj obj)
{
	static navi_obj port = { .n = NAVI_VOID_TAG };
	static navi_env env = {0};
	if (navi_is_void(port)) {
		env = navi_empty_environment();
		port = navi_make_file_output_port(stdout);
		navi_gc_guard(port, env);
	}
	navi_port_write(navi_port(port), obj, env);
}*/

static __hot void navi_free(struct navi_object *obj)
{
	gc_stats.bytes -= sizeof(struct navi_object) + object_size(obj);
	gc_stats.objects--;
	switch (obj->type) {
	case NAVI_PAIR:
	case NAVI_PARAMETER:
		navi_slab_free(pair_cache, obj);
		return;
	case NAVI_THUNK:
	case NAVI_BOUNCE:
		navi_env_unref(navi_thunk(to_obj(obj))->env);
		break;
	case NAVI_ENVIRONMENT:
		navi_env_unref(navi_environment(to_obj(obj)));
		break;
	case NAVI_SYMBOL:
		// remove interned symbols from symbol table
		if (navi_symbol_is_interned(to_obj(obj)))
			NAVI_LIST_REMOVE(navi_symbol(to_obj(obj)), link);
		break;
	default:
		break;
	}
	obj->type = NAVI_TRAP;
	free(obj);
}

static __hot __const unsigned long symbol_hash(const char *symbol)
{
	unsigned long hash = 5381;
	int c;

	while ((c = *symbol++))
		hash = ((hash << 5) + hash) ^ c; // hash * 33 ^ c

	return hash;
}

static void symbol_table_init(void)
{
	for (unsigned i = 0; i < SYMTAB_SIZE; i++)
		NAVI_LIST_INIT(&symbol_table[i]);

	#define intern(cname, name) \
		navi_gc_protect((cname = navi_make_symbol(name)))
	intern(navi_sym_begin,           "begin");
	intern(navi_sym_quote,           "quote");
	intern(navi_sym_quasiquote,      "quasiquote");
	intern(navi_sym_unquote,         "unquote");
	intern(navi_sym_splice,          "unquote-splice");
	intern(navi_sym_else,            "else");
	intern(navi_sym_eq_lt,           "=>");
	intern(navi_sym_lib_paths,       "#lib-search-paths");
	intern(navi_sym_command_line,    "#command-line");
	intern(navi_sym_current_exn,     "#current-exception-handler");
	intern(navi_sym_current_input,   "current-input-port");
	intern(navi_sym_current_output,  "current-output-port");
	intern(navi_sym_current_error,   "current-error-port");
	intern(navi_sym_read_error,      "#read-error");
	intern(navi_sym_file_error,      "#file-error");
	intern(navi_sym_internal_error,  "#internal-error");
	intern(navi_sym_repl,            "#repl");
	intern(navi_sym_export,          "export");
	intern(navi_sym_import,          "import");
	intern(navi_sym_only,            "only");
	intern(navi_sym_except,          "except");
	intern(navi_sym_prefix,          "prefix");
	intern(navi_sym_rename,          "rename");
	intern(navi_sym_deflib,          "define-library");
	intern(navi_sym_include,         "include");
	intern(navi_sym_include_ci,      "include-ci");
	intern(navi_sym_include_libdecl, "include-library-declarations");
	intern(navi_sym_cond_expand,     "cond-expand");
	intern(navi_sym_ellipsis,        "...");
	intern(navi_sym_underscore,      "_");
	#undef intern
}

void navi_init(void)
{
	pair_cache = navi_slab_cache_create(
		sizeof(struct navi_object) + sizeof(struct navi_pair), 0);
	guard_cache = navi_slab_cache_create(
		sizeof(struct navi_guard), NAVI_SLAB_DOUBLY_LINKED);
	binding_cache = navi_slab_cache_create(
		sizeof(struct navi_binding), NAVI_SLAB_DOUBLY_LINKED);
	symbol_table_init();
	navi_internal_init();
}

static navi_obj symbol_lookup(const char *str, unsigned long hashcode)
{
	struct navi_symbol *it;
	struct sym_bucket *head = &symbol_table[hashcode % SYMTAB_SIZE];

	NAVI_LIST_FOREACH(it, head, link) {
		if (!strcmp(it->data, str))
			return to_obj(navi_object(it));
	}
	return navi_make_void();
}

static void register_object(struct navi_object *obj, size_t size)
{
	NAVI_SLIST_INSERT_HEAD(&heap, obj, link);
	gc_stats.bytes += size;
	gc_stats.objects++;
}

static navi_obj slab_make_object(struct slab_cache *cache, enum navi_type type)
{
	struct navi_object *obj = navi_slab_alloc(cache);
	obj->type = type;
	register_object(obj, cache->obj_size);
	return to_obj(obj);
}

static navi_obj make_object(enum navi_type type, size_t size)
{
	struct navi_object *obj = navi_critical_malloc(sizeof(struct navi_object) + size);
	obj->type = type;
	register_object(obj, sizeof(struct navi_object) + size);
	return to_obj(obj);
}

navi_obj navi_make_uninterned(const char *str)
{
	size_t len = strlen(str);
	navi_obj obj = make_object(NAVI_SYMBOL,
			sizeof(struct navi_symbol) + len + 1);
	struct navi_symbol *symbol = navi_symbol(obj);

	for (size_t i = 0; i < len+1; i++)
		symbol->data[i] = str[i];
	symbol->link.le_prev = NULL;
	return obj;
}

DEFUN(gensym, "gensym", 0, 0)
{
	char buf[64];
	static unsigned count = 0;

	snprintf(buf, 64, "g%u", count++);
	buf[63] = '\0';
	return navi_make_uninterned(buf);
}

navi_obj _navi_make_named_parameter(navi_obj symbol, navi_obj converter)
{
	navi_obj param = navi_make_pair(symbol, converter);
	param.p->type = NAVI_PARAMETER;
	return param;
}

/*
 * XXX: this is only half of the Scheme make-parameter procedure.  It is still
 * necessary to bind something to 'key' in the dynamic environment after
 * calling this procedure.
 */
navi_obj _navi_make_parameter(navi_obj converter)
{
	char buf[64];
	static unsigned count = 0;
	snprintf(buf, 64, "param%u", count++);
	buf[63] = '\0';
	return _navi_make_named_parameter(navi_make_uninterned(buf), converter);
}

/* Only called if symbol doesn't already exist */
static navi_obj new_symbol(const char *str, unsigned long hashcode)
{
	navi_obj object = navi_make_uninterned(str);
	struct navi_symbol *symbol = navi_symbol(object);
	NAVI_LIST_INSERT_HEAD(&symbol_table[hashcode % SYMTAB_SIZE], symbol, link);
	return object;
}

navi_obj navi_make_symbol(const char *str)
{
	unsigned long hashcode = symbol_hash(str);
	navi_obj symbol = symbol_lookup(str, hashcode);

	return navi_is_void(symbol) ? new_symbol(str, hashcode) : symbol;
}

#define SLOW
struct navi_binding *navi_make_binding(navi_obj symbol, navi_obj object)
{
	struct navi_binding *binding = navi_slab_alloc(binding_cache);
	binding->symbol = symbol;
	binding->object = object;
	return binding;
}

void navi_scope_free(struct navi_scope *scope)
{
	struct navi_binding *binding, *n;
	NAVI_LIST_REMOVE(scope, link);
	navi_scope_for_each_safe(binding, n, scope) {
		NAVI_LIST_REMOVE(binding, link);
		navi_slab_free(binding_cache, binding);
	}
	if (scope->next != NULL)
		_navi_scope_unref(scope->next);
	free(scope);
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
	navi_obj obj = make_object(NAVI_STRING, sizeof(struct navi_string));
	struct navi_string *str = navi_string(obj);
	str->data = navi_critical_malloc(capacity + 1);
	str->data[capacity] = '\0';
	str->data[size] = '\0';
	str->capacity = capacity;
	str->size = size;
	str->length = length;
	return obj;
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

navi_obj navi_make_empty_pair(void)
{
	return slab_make_object(pair_cache, NAVI_PAIR);
}

navi_obj navi_make_pair(navi_obj car, navi_obj cdr)
{
	navi_obj obj = navi_make_empty_pair();
	navi_set_car(obj, car);
	navi_set_cdr(obj, cdr);
	return obj;
}

navi_obj navi_make_port(
		int(*read_u8)(struct navi_port*, navi_env),
		void(*write_u8)(uint8_t,struct navi_port*, navi_env),
		long(*read_char)(struct navi_port*, navi_env),
		void(*write_char)(long, struct navi_port*, navi_env),
		void(*close_in)(struct navi_port*, navi_env),
		void(*close_out)(struct navi_port*, navi_env),
		void *specific)
{
	navi_obj obj = make_object(NAVI_PORT, sizeof(struct navi_port));
	struct navi_port *port = navi_port(obj);
	port->read_u8 = read_u8;
	port->write_u8 = write_u8;
	port->read_char = read_char;
	port->write_char = write_char;
	port->close_in = close_in;
	port->close_out = close_out;
	port->flags = 0;
	port->expr = navi_make_void();
	port->pos = 0;
	port->specific = specific;
	return obj;
}

navi_obj navi_make_vector(size_t size)
{
	navi_obj obj = make_object(NAVI_VECTOR,
			sizeof(struct navi_vector) + sizeof(navi_obj)*size);
	navi_vector(obj)->size = size;
	return obj;
}

navi_obj navi_make_bytevec(size_t size)
{
	navi_obj obj = make_object(NAVI_BYTEVEC,
			sizeof(struct navi_bytevec) + size + 1);
	struct navi_bytevec *vec = navi_bytevec(obj);
	vec->size = size;
	vec->data[size] = '\0';
	return obj;
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
	navi_obj obj = make_object(NAVI_PROCEDURE, sizeof(struct navi_procedure));
	struct navi_procedure *proc = navi_procedure(obj);
	proc->name = name;
	proc->args = args;
	proc->body = body;
	proc->env = env.lexical;
	proc->arity = count_pairs(args);
	proc->flags = 0;
	proc->types = NULL;
	if (!navi_is_proper_list(args))
		proc->flags |= NAVI_PROC_VARIADIC;
	navi_env_ref(env);
	return obj;
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
	return make_object(NAVI_ESCAPE, sizeof(struct navi_escape));
}

navi_obj navi_capture_env(navi_env env)
{
	navi_obj obj = make_object(NAVI_ENVIRONMENT, sizeof(navi_env));
	memcpy(obj.p->data, &env, sizeof(env));
	return obj;
}

navi_obj navi_make_thunk(navi_obj expr, navi_env env)
{
	navi_obj obj = make_object(NAVI_THUNK, sizeof(struct navi_thunk));
	struct navi_thunk *thunk = navi_thunk(obj);
	thunk->expr = expr;
	thunk->env = env;
	navi_env_ref(env);
	return obj;
}

static navi_obj proc_from_spec(const struct navi_spec *spec, navi_env env)
{
	navi_obj obj = make_object(spec->type, sizeof(struct navi_procedure));
	struct navi_procedure *proc = navi_procedure(obj);
	memcpy(proc, &spec->proc, sizeof(*proc));
	proc->name = navi_make_symbol(spec->ident);
	proc->args = navi_make_symbol("scm_args");
	proc->env = env.lexical;
	return obj;
}

static navi_obj parameter_from_spec(const struct navi_spec *spec, navi_env env)
{
	navi_obj name = navi_make_symbol(spec->ident);
	navi_obj value = navi_from_spec(spec->param_value, env);
	navi_obj converter = navi_from_spec(spec->param_converter, env);
	navi_obj r = navi_make_named_parameter(name, value, converter, env);
	return r;
}

static navi_obj list_from_spec(const struct navi_spec *spec, navi_env env)
{
	struct navi_pair head, *ptr = &head;
	for (size_t i = 0; spec->elms[i]; i++) {
		ptr->cdr = navi_make_empty_pair();
		ptr = navi_pair(ptr->cdr);
		ptr->car = navi_from_spec(spec->elms[i], env);
	}
	ptr->cdr = navi_make_nil();
	return head.cdr;
}

navi_obj navi_from_spec(const struct navi_spec *spec, navi_env env)
{
	if (spec->init)
		return spec->init(spec);
	switch (spec->type) {
	case NAVI_EOF:
		return navi_make_eof();
	case NAVI_NIL:
		return navi_make_nil();
	case NAVI_FIXNUM:
		return navi_make_fixnum(spec->num);
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
	case NAVI_BYTEVEC:
		return navi_make_bytevec(spec->size);
	case NAVI_MACRO:
	case NAVI_SPECIAL:
	case NAVI_PROCEDURE:
		return proc_from_spec(spec, env);
	case NAVI_PARAMETER:
		return parameter_from_spec(spec, env);
	case NAVI_LIST:
		return list_from_spec(spec, env);
	default:
		break;
	}
	navi_die("navi_from_spec: unknown or unsupported type");
}

int navi_eqvp(navi_obj fst, navi_obj snd)
{
	if (navi_type(fst) != navi_type(snd))
		return false;

	switch (navi_type(fst)) {
	case NAVI_VOID:
	case NAVI_NIL:
	case NAVI_EOF:
		return true;
	case NAVI_FIXNUM:
		return navi_fixnum(fst) == navi_fixnum(snd);
	case NAVI_BOOL:
		return navi_bool(fst) ? navi_bool(snd) : !navi_bool(snd);
	case NAVI_CHAR:
		return navi_char(fst) == navi_char(snd);
	case NAVI_PAIR:
	case NAVI_PORT:
	case NAVI_SYMBOL:
	case NAVI_VECTOR:
	case NAVI_STRING:
	case NAVI_VALUES:
	case NAVI_BYTEVEC:
	case NAVI_THUNK:
	case NAVI_MACRO:
	case NAVI_SPECIAL:
	case NAVI_PROMISE:
	case NAVI_PROCEDURE:
	case NAVI_CASELAMBDA:
	case NAVI_ESCAPE:
	case NAVI_PARAMETER:
	case NAVI_ENVIRONMENT:
	case NAVI_BOUNCE:
		return fst.p == snd.p;
	case NAVI_TRAP:
		navi_die("trap!");
	}
	navi_die("navi_eqvp: unknown type");
}

DEFUN(eqvp, "eqv?", 2, 0, NAVI_ANY, NAVI_ANY)
{
	return navi_make_bool(navi_eqvp(scm_arg1, scm_arg2));
}

DEFUN(eqp, "eq?", 2, 0, NAVI_ANY, NAVI_ANY)
{
	return navi_make_bool(navi_eqp(scm_arg1, scm_arg2));
}

static bool list_equal(navi_obj fst, navi_obj snd)
{
	navi_obj cons_a, cons_b;
	navi_list_for_each_zipped(cons_a, cons_b, fst, snd) {
		if (!navi_equalp(navi_car(cons_a), navi_car(cons_b)))
			return false;
	}
	if (!navi_equalp(cons_a, cons_b))
		return false;
	return true;
}

static bool vector_equal(navi_obj fst, navi_obj snd)
{
	struct navi_vector *a = navi_vector(fst);
	struct navi_vector *b = navi_vector(snd);
	if (a->size != b->size)
		return false;
	for (size_t i = 0; i < a->size; i++) {
		if (!navi_equalp(a->data[i], b->data[i]))
			return false;
	}
	return true;
}

static bool bytevec_equal(navi_obj fst, navi_obj snd)
{
	struct navi_bytevec *a = navi_bytevec(fst);
	struct navi_bytevec *b = navi_bytevec(snd);
	if (a->size != b->size)
		return false;
	for (size_t i = 0; i < a->size; i++) {
		if (a->data[i] != b->data[i])
			return false;
	}
	return true;
}

// FIXME: doesn't terminate on circular data structures
int navi_equalp(navi_obj fst, navi_obj snd)
{
	if (navi_type(fst) != navi_type(snd))
		return false;
	switch(navi_type(fst)) {
	case NAVI_VOID:
	case NAVI_NIL:
	case NAVI_EOF:
		return true;
	case NAVI_FIXNUM:
		return navi_fixnum(fst) == navi_fixnum(snd);
	case NAVI_BOOL:
		return navi_bool(fst) ? navi_bool(snd) : !navi_bool(snd);
	case NAVI_CHAR:
		return navi_char(fst) == navi_char(snd);
	case NAVI_PORT:
	case NAVI_SYMBOL:
	case NAVI_VALUES:
	case NAVI_THUNK:
	case NAVI_MACRO:
	case NAVI_SPECIAL:
	case NAVI_PROMISE:
	case NAVI_PROCEDURE:
	case NAVI_CASELAMBDA:
	case NAVI_ESCAPE:
	case NAVI_PARAMETER:
	case NAVI_ENVIRONMENT:
	case NAVI_BOUNCE:
		return fst.p == snd.p;
	case NAVI_PAIR:
		return list_equal(fst, snd);
	case NAVI_VECTOR:
		return vector_equal(fst, snd);
	case NAVI_STRING:
		return navi_string_equal(fst, snd);
	case NAVI_BYTEVEC:
		return bytevec_equal(fst, snd);
	case NAVI_TRAP:
		navi_die("trap!");
	}
	navi_die("navi_equalp: unknown type");
}

DEFUN(equalp, "equal?", 2, 0, NAVI_ANY, NAVI_ANY)
{
	return navi_make_bool(navi_equalp(scm_arg1, scm_arg2));
}

__hot struct navi_guard *navi_gc_guard(navi_obj obj, navi_env env)
{
	if (navi_is_immediate(obj))
		return NULL;

	struct navi_guard *guard = navi_slab_alloc(guard_cache);
	guard->obj = obj;
	NAVI_LIST_INSERT_HEAD(&env.lexical->guards, guard, link);
	return guard;
}

__hot void navi_gc_unguard(struct navi_guard *guard)
{
	if (guard) {
		NAVI_LIST_REMOVE(guard, link);
		navi_slab_free(guard_cache, guard);
	}
}

static inline bool gc_is_protected(struct navi_object *obj)
{
	return obj->flags & NAVI_GC_PROTECT;
}

static inline bool gc_is_marked(struct navi_object *obj)
{
	return obj->flags & NAVI_GC_MARK;
}

static inline void gc_set_mark(navi_obj obj)
{
	obj.p->flags |= NAVI_GC_MARK;
}

static inline void gc_clear_mark(struct navi_object *obj)
{
	obj->flags &= ~NAVI_GC_MARK;
}
static __hot void gc_mark_obj(navi_obj obj)
{
	struct navi_vector *vec;
	struct navi_procedure *proc;

	if (!navi_ptr_type(obj))
		return;
	switch (navi_type(obj)) {
	case NAVI_VOID:
	case NAVI_NIL:
	case NAVI_EOF:
	case NAVI_FIXNUM:
	case NAVI_BOOL:
	case NAVI_CHAR:
		break;
	case NAVI_PAIR:
	case NAVI_BOUNCE:
	case NAVI_PARAMETER:
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
	case NAVI_THUNK:
		gc_set_mark(obj);
		gc_mark_obj(navi_thunk(obj)->expr);
		break;
	case NAVI_MACRO:
	case NAVI_SPECIAL:
	case NAVI_PROMISE:
	case NAVI_PROCEDURE:
		gc_set_mark(obj);
		proc = navi_procedure(obj);
		gc_mark_obj(proc->args);
		gc_set_mark(proc->name);
		if (!navi_proc_is_builtin(proc))
			gc_mark_obj(proc->body);
		break;
	case NAVI_ESCAPE:
		gc_set_mark(obj);
		gc_mark_obj(navi_escape(obj)->arg);
		break;
	case NAVI_ENVIRONMENT:
		gc_set_mark(obj);
		break;
	case NAVI_TRAP:
		navi_die("trap!");
	}
}

static void gc_mark_env(struct navi_scope *env)
{
	struct navi_binding *binding;
	navi_scope_for_each(binding, env) {
		gc_set_mark(binding->symbol);
		gc_mark_obj(binding->object);
	}
	struct navi_guard *guard;
	NAVI_LIST_FOREACH(guard, &env->guards, link) {
		gc_mark_obj(guard->obj);
	}
}

static void gc_mark(void)
{
	struct navi_scope *scope;
	NAVI_LIST_FOREACH(scope, &active_environments, link) {
		gc_mark_env(scope);
	}
}

static void gc_sweep(void)
{
	struct navi_object *obj, *last, *p;
	last = heap.slh_first;
	NAVI_SLIST_FOREACH_SAFE(obj, &heap, link, p) {
		if (gc_is_marked(obj)) {
			gc_clear_mark(obj);
			last = obj;
		} else if (unlikely(gc_is_protected(obj))) {
			last = obj;
		} else if (obj->type == NAVI_SYMBOL) {
			last = obj;
		} else {
			if (unlikely(obj == NAVI_SLIST_FIRST(&heap)))
				NAVI_SLIST_REMOVE_HEAD(&heap, link);
			else
				NAVI_SLIST_REMOVE_AFTER(last, link);
			navi_free(obj);
		}
	}
}

unsigned int _navi_gc_disabled = 0;

static void do_gc_collect(void)
{
	gc_mark();
	gc_sweep();
	gc_stats.threshold = gc_stats.bytes * 4;
}

void navi_gc_collect(void)
{
	if (!_navi_gc_disabled) {
		do_gc_collect();
	}
}

void navi_gc_check(void)
{
	// if threshold unset, trigger first collection
	if (unlikely(!gc_stats.threshold))
		gc_stats.threshold = gc_stats.bytes - 1;
	if (likely(gc_stats.bytes < gc_stats.threshold))
		return;
	if (unlikely(_navi_gc_disabled))
		return;
	do_gc_collect();
}

DEFUN(gc_collect, "gc-collect", 0, 0)
{
	navi_gc_collect();
	return navi_unspecified();
}

DEFUN(gc_count, "gc-count", 0, 0)
{
	struct navi_object *expr;
	NAVI_SLIST_FOREACH(expr, &heap, link) {
		if (navi_is_builtin(to_obj(expr)))
			continue;
		printf("<%p> ", (void*)expr);
		navi_write(to_obj(expr), scm_env);
		putchar('\n');
	}
	return navi_unspecified();
}

DEFUN(gc_stats, "gc-stats", 0, 0)
{
	char buf[128];
	struct navi_port *p = navi_port(navi_current_output_port(scm_env));
	snprintf(buf, 127,
			"  Bytes allocated: %lu\n"
			"(without headers): %lu\n"
			"Objects allocated: %lu\n"
			"        Threshold: %lu\n",
			gc_stats.bytes,
			gc_stats.bytes - gc_stats.objects*sizeof(struct navi_object),
			gc_stats.objects,
			gc_stats.threshold);
	buf[127] = '\0';
	navi_port_write_cstr(buf, p, scm_env);
	return navi_unspecified();
}
