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

#include <stdlib.h>
#include <string.h>

#include "navi.h"

#include "default_bindings.c"

NAVI_LIST_HEAD(active_environments);

/* FIXME: this is a REALLY bad hash function! */
static unsigned long ptr_hash(navi_obj ptr)
{
	return ptr.n;
}

static struct navi_hlist_head *get_bucket(struct navi_scope *scope,
		unsigned long hashcode)
{
	return &scope->bindings[hashcode % NAVI_ENV_HT_SIZE];
}

static struct navi_binding *make_binding(navi_obj symbol, navi_obj object)
{
	struct navi_binding *binding = navi_critical_malloc(sizeof(struct navi_binding));
	binding->symbol = symbol;
	binding->object = object;
	return binding;
}

static struct navi_binding *scope_lookup(struct navi_scope *scope,
		navi_obj symbol, unsigned long hashcode)
{
	struct navi_binding *binding;
	struct navi_hlist_head *hd = get_bucket(scope, hashcode);
	
	navi_hlist_for_each_entry(binding, hd, struct navi_binding, chain) {
		if (binding->symbol.p == symbol.p)
			return binding;
	}
	return NULL;
}

struct navi_binding *navi_scope_lookup(struct navi_scope *scope, navi_obj symbol)
{
	return scope_lookup(scope, symbol, ptr_hash(symbol));
}

struct navi_binding *navi_env_binding(struct navi_scope *env, navi_obj symbol)
{
	struct navi_binding *binding;
	unsigned long hashcode = ptr_hash(symbol);

	for (struct navi_scope *s = env; s; s = s->next) {
		binding = scope_lookup(s, symbol, hashcode);
		if (binding != NULL)
			return binding;
	}
	return NULL;
}

static inline struct navi_scope *make_scope(void)
{
	struct navi_scope *scope = navi_critical_malloc(sizeof(struct navi_scope));
	for (unsigned i = 0; i < NAVI_ENV_HT_SIZE; i++)
		NAVI_INIT_HLIST_HEAD(&scope->bindings[i]);
	navi_clist_add(&scope->chain, &active_environments);
	scope->refs = 1;
	scope->next = NULL;
	return scope;
}

navi_env navi_env_new_scope(navi_env env)
{
	struct navi_scope *scope = make_scope();
	navi_env_ref(env);
	scope->next = env.lexical;
	return (navi_env) { .lexical = scope, .dynamic = env.dynamic };
}

navi_env navi_dynamic_env_new_scope(navi_env env)
{
	struct navi_scope *scope = make_scope();
	navi_env_ref(env);
	scope->next = env.dynamic;
	return (navi_env) { .lexical = env.lexical, .dynamic = scope };
}

void env_set(navi_env env, navi_obj symbol, navi_obj object)
{
	struct navi_binding *binding;
	struct navi_hlist_head *head;

	binding = navi_env_binding(env.lexical, symbol);
	if (binding) {
		binding->object = object;
		return;
	}

	/* FIXME: hash() already computed in navi_env_binding */
	head = get_bucket(env.lexical, ptr_hash(symbol));
	binding = make_binding(symbol, object);
	navi_hlist_add_head(&binding->chain, head);
}

void navi_scope_set(struct navi_scope *env, navi_obj symbol, navi_obj object)
{
	struct navi_binding *binding;
	unsigned long hashcode = ptr_hash(symbol);

	if ((binding = scope_lookup(env, symbol, hashcode)) != NULL) {
		binding->object = object;
		return;
	}

	binding = make_binding(symbol, object);
	navi_hlist_add_head(&binding->chain, get_bucket(env, hashcode));
}

int navi_scope_unset(struct navi_scope *env, navi_obj symbol)
{
	struct navi_binding *binding;
	unsigned long hashcode = ptr_hash(symbol);

	if ((binding = scope_lookup(env, symbol, hashcode)) == NULL)
		return 0;

	navi_hlist_del(&binding->chain);
	return 1;
}

/* XXX: assumes we're executing in env */
navi_env navi_extend_environment(navi_env env, navi_obj vars, navi_obj args)
{
	navi_obj vcons, acons;
	navi_env new = navi_env_new_scope(env);

	navi_list_for_each_zipped(vcons, acons, vars, args) {
		navi_scope_set(new.lexical, navi_car(vcons), navi_car(acons));
	}
	/* dotted tail */
	if (!navi_is_nil(vcons)) {
		navi_scope_set(new.lexical, vcons, acons);
	}
	return new;
}

static void navi_import_all(struct navi_scope *dst, struct navi_scope *src)
{
	for (unsigned i = 0; i < NAVI_ENV_HT_SIZE; i++) {
		struct navi_binding *binding;
		navi_hlist_for_each_entry(binding, &src->bindings[i],
				struct navi_binding, chain) {
			navi_scope_set(dst, binding->symbol, binding->object);
		}
	}
}

navi_env _navi_empty_environment(void)
{
	return (navi_env) { .lexical = make_scope(), .dynamic = make_scope() };
}

/*
 * Make "empty" environment, which has only (import ...).
 */
navi_env navi_empty_environment(void)
{
	navi_env env = _navi_empty_environment();
	navi_scope_set(env.lexical, navi_sym_import,
			navi_from_spec(&SCM_DECL(import), env));
	return env;
}

static navi_env new_lexical_environment(navi_env env)
{
	return (navi_env) {
		.lexical = make_scope(),
		.dynamic = _navi_scope_ref(env.dynamic)
	};
}

void navi_scope_free(struct navi_scope *scope)
{
	struct navi_binding *binding;
	struct navi_hlist_node *n;
	navi_clist_del(&scope->chain);
	navi_scope_for_each_safe(binding, n, scope) {
		navi_hlist_del(&binding->chain);
		free(binding);
	}
	if (scope->next != NULL)
		_navi_scope_unref(scope->next);
}

static struct navi_scope *get_global_scope(struct navi_scope *s)
{
	for (; s->next; s = s->next);
	return s;
}

navi_env navi_get_global_env(navi_env env)
{
	return (navi_env) {
		.lexical = get_global_scope(env.lexical),
		.dynamic = get_global_scope(env.dynamic)
	};
}

/* Lexical Bindings {{{ */
static navi_obj eval_defvar(navi_obj sym, navi_obj rest, navi_env env)
{
	if (navi_type(navi_cdr(rest)) != NAVI_NIL)
		navi_arity_error(env, navi_make_symbol("define"));

	navi_scope_set(env.lexical, sym, navi_eval(navi_car(rest), env));
	return navi_unspecified();
}

static navi_obj eval_defun(navi_obj fundecl, navi_obj rest, navi_env env)
{
	navi_obj proc, name;

	if (!navi_is_list_of(fundecl, NAVI_SYMBOL, true))
		navi_error(env, "invalid defun list");

	name = navi_car(fundecl);
	proc = navi_make_procedure(navi_cdr(fundecl), rest, name, env);
	navi_scope_set(env.lexical, name, proc);
	return navi_unspecified();
}

DEFSPECIAL(define, "define", 2, NAVI_PROC_VARIADIC, NAVI_ANY, NAVI_ANY)
{
	enum navi_type type = navi_type(scm_arg1);
	if (type == NAVI_SYMBOL)
		return eval_defvar(scm_arg1, navi_cdr(scm_args), scm_env);
	if (type == NAVI_PAIR)
		return eval_defun(scm_arg1, navi_cdr(scm_args), scm_env);
	navi_error(scm_env, "invalid define list");
}

DEFSPECIAL(define_syntax, "define-syntax", 2, 0, NAVI_SYMBOL, NAVI_ANY)
{
	navi_obj transformer = navi_eval(scm_arg2, scm_env);
	navi_type_check(transformer, NAVI_MACRO, scm_env);
	navi_scope_set(scm_env.lexical, scm_arg1, transformer);
	return navi_unspecified();
}

static void extend_with_values(navi_obj vars, navi_obj vals, navi_obj which, navi_env env)
{
	navi_obj cons;
	size_t i = 0;

	if (navi_type(vals) != NAVI_VALUES) {
		if (navi_list_length(vars) != 1)
			navi_arity_error(env, which);
		navi_scope_set(env.lexical, navi_car(vars), vals);
		return;
	}

	if ((size_t)navi_list_length(vars) != navi_vector_length(vals))
		navi_arity_error(env, which);

	navi_list_for_each(cons, vars) {
		navi_scope_set(env.lexical, navi_car(cons), navi_vector_ref(vals, i++));
	}
}

DEFSPECIAL(define_values, "define-values", 2, 0, NAVI_PROPER_LIST, NAVI_ANY)
{
	extend_with_values(scm_arg1, navi_eval(scm_arg2, scm_env),
			navi_make_symbol("define-values"), scm_env);
	return navi_unspecified();
}

DEFSPECIAL(defmacro, "defmacro", 2, NAVI_PROC_VARIADIC, NAVI_PAIR, NAVI_ANY)
{
	navi_obj macro, name;

	if (!navi_is_list_of(scm_arg1, NAVI_SYMBOL, true))
		navi_error(scm_env, "invalid define-macro list");

	name = navi_car(scm_arg1);
	macro = navi_make_macro(navi_cdr(scm_arg1), navi_cdr(scm_args), name, scm_env);
	navi_scope_set(scm_env.lexical, name, macro);
	return navi_unspecified();
}

static bool let_def_valid(navi_obj def)
{
	return navi_type(def) == NAVI_PAIR &&
		navi_type(navi_cdr(def)) == NAVI_PAIR &&
		navi_type(navi_cddr(def)) == NAVI_NIL &&
		navi_type(navi_car(def)) == NAVI_SYMBOL;
}

static bool let_defs_valid(navi_obj list)
{
	navi_obj cons;

	navi_list_for_each(cons, list) {
		if (!let_def_valid(navi_car(cons)))
			return false;
	}
	return navi_type(cons) == NAVI_NIL;
}

static bool let_values_def_valid(navi_obj def)
{
	return navi_type(def) == NAVI_PAIR &&
		navi_type(navi_cdr(def)) == NAVI_PAIR &&
		navi_type(navi_cddr(def)) == NAVI_NIL &&
		navi_is_list_of(navi_car(def), NAVI_SYMBOL, false);
}

static bool letvals_defs_valid(navi_obj list)
{
	navi_obj cons;

	navi_list_for_each(cons, list) {
		if (!let_values_def_valid(navi_car(cons)))
			return false;
	}
	return navi_type(cons) == NAVI_NIL;
}

static navi_env let_extend_env(navi_obj def_list, navi_env env)
{
	navi_obj cons;
	navi_env new = navi_env_new_scope(env);

	navi_list_for_each(cons, def_list) {
		navi_obj defn = navi_car(cons);
		navi_obj val = navi_eval(navi_cadr(defn), env);
		navi_scope_set(new.lexical, navi_car(defn), val);
	}

	return new;
}

static navi_env sequential_let_extend_env(navi_obj def_list, navi_env env)
{
	navi_obj cons;
	navi_env new = navi_env_new_scope(env);

	navi_list_for_each(cons, def_list) {
		navi_obj defn = navi_car(cons);
		navi_obj val = navi_eval(navi_cadr(defn), new);
		navi_scope_set(new.lexical, navi_car(defn), val);
	}

	return new;
}

static navi_env letvals_extend_env(navi_obj def_list, navi_env env)
{
	navi_obj cons;
	navi_env new = navi_env_new_scope(env);

	navi_list_for_each(cons, def_list) {
		navi_obj vals = navi_eval(navi_cadar(cons), env);
		extend_with_values(navi_caar(cons), vals,
				navi_make_symbol("let-values"), env);
	}

	return new;
}

#define DEFLET(name, scmname, validate, extend)                              \
	DEFSPECIAL(name, scmname, 2, NAVI_PROC_VARIADIC,                     \
			NAVI_ANY, NAVI_ANY)                                  \
	{                                                                    \
		navi_obj result;                                             \
		navi_env new_env;                                            \
		                                                             \
		if (!validate(scm_arg1))                                     \
			navi_error(scm_env, "invalid " scmname " list");     \
		                                                             \
		new_env = extend(scm_arg1, scm_env);                         \
		result = scm_begin(0, navi_cdr(scm_args), new_env, NULL);    \
		navi_env_unref(new_env);                                     \
		return result;                                               \
	}

DEFLET(let, "let", let_defs_valid, let_extend_env)
DEFLET(sequential_let, "let*", let_defs_valid, sequential_let_extend_env)
DEFLET(let_values, "let-values", letvals_defs_valid, letvals_extend_env)

DEFSPECIAL(set, "set!", 2, 0, NAVI_SYMBOL, NAVI_ANY)
{
	struct navi_binding *binding;
	navi_obj value;

	binding = navi_env_binding(scm_env.lexical, scm_arg1);
	if (binding == NULL)
		navi_unbound_identifier_error(scm_env, scm_arg1);

	value = navi_eval(scm_arg2, scm_env);
	binding->object = value;

	return navi_unspecified();
}
/* Lexical Bindings }}} */
/* Dynamic Bindings {{{ */

navi_obj navi_parameter_lookup(navi_obj param, navi_env env)
{
	struct navi_binding *binding;
	binding = navi_env_binding(env.dynamic, navi_parameter_key(param));
	if (!binding)
		// XXX: this is a critical error and should never happen
		navi_error(env, "unbound parameter object");
	return binding->object;
}

navi_obj navi_parameter_convert(navi_obj param, navi_obj obj, navi_env env)
{
	navi_obj args = navi_make_pair(obj, navi_make_nil());
	navi_obj converter = navi_parameter_converter(param);
	if (navi_is_void(converter))
		return obj;
	return navi_force_tail(navi_apply(navi_procedure(converter), args, env), env);
}

navi_obj navi_make_parameter(navi_obj value, navi_obj converter, navi_env env)
{
	navi_obj param = _navi_make_parameter(converter);
	navi_scope_set(get_global_scope(env.dynamic), navi_parameter_key(param),
			navi_parameter_convert(param, value, env));
	return param;
}

navi_obj navi_make_named_parameter(navi_obj symbol, navi_obj value,
		navi_obj converter, navi_env env)
{
	navi_obj param = _navi_make_named_parameter(symbol, converter);
	navi_scope_set(get_global_scope(env.dynamic), navi_parameter_key(param),
			navi_parameter_convert(param, value, env));
	return param;
}

DEFUN(make_parameter, "make-parameter", 1, NAVI_PROC_VARIADIC, NAVI_ANY)
{
	navi_obj converter = navi_make_void();
	if (scm_nr_args > 2)
		navi_arity_error(scm_env, navi_make_symbol("make-parameter"));
	if (scm_nr_args == 2)
		converter = scm_arg2;
	return navi_make_parameter(scm_arg1, converter, scm_env);
}

static navi_env parameterize_extend_env(navi_obj defs, navi_env env)
{
	navi_env new;
	navi_obj cons, params = navi_make_nil();
	if (!navi_is_pair(defs))
		navi_error(env, "invalid syntax in parameterize");

	// first, eval the params and make sure they're actually parameters
	navi_list_for_each(cons, defs) {
		navi_obj param, def = navi_car(cons);
		if (navi_list_length_safe(def) != 2)
			navi_error(env, "invalid syntax in parameterize");
		param = navi_eval(navi_car(def), env);
		if (!navi_is_parameter(param))
			navi_error(env, "non-parameter in parameterize");
		params = navi_make_pair(navi_make_pair(param, navi_cadr(def)), params);
	}
	if (!navi_is_nil(cons))
		navi_error(env, "not a proper list");

	// then evaluate the values and bind them to the parameters
	new = navi_dynamic_env_new_scope(env);
	navi_list_for_each(cons, params) {
		navi_obj def = navi_car(cons);
		navi_obj param = navi_car(def);
		navi_obj val = navi_eval(navi_cdr(def), env);
		val = navi_parameter_convert(param, val, env);
		navi_scope_set(new.dynamic, navi_parameter_key(param), val);
	}
	return new;
}

DEFSPECIAL(parameterize, "parameterize", 2, NAVI_PROC_VARIADIC,
		NAVI_ANY, NAVI_ANY)
{
	navi_env new_env = parameterize_extend_env(scm_arg1, scm_env);
	navi_obj result = scm_begin(0, navi_cdr(scm_args), new_env, NULL);
	navi_env_unref(new_env);
	return result;
}

/* Dynamic Bindings }}} */
/* Libraries {{{ */
static bool libraries_initialized = false;
static struct navi_hlist_head libraries[NAVI_ENV_HT_SIZE];

static void libraries_init(void)
{
	for (int i = 0; i < NAVI_ENV_HT_SIZE; i++)
		NAVI_INIT_HLIST_HEAD(&libraries[i]);
	libraries_initialized = true;
}

static unsigned long libname_hash(navi_obj name)
{
	navi_obj cons;
	uintptr_t hash = (uintptr_t) navi_car(name).p;
	navi_list_for_each(cons, navi_cdr(name)) {
		hash ^= ((uintptr_t) navi_car(cons).p) >> 1;
	}
	return hash;
}

static bool libname_equal(navi_obj a, navi_obj b)
{
	navi_obj cons_a, cons_b;
	navi_list_for_each_zipped(cons_a, cons_b, a, b) {
		if (navi_car(cons_a).p != navi_car(cons_b).p)
			return false;
	}
	return navi_is_nil(cons_a) && navi_is_nil(cons_b);
}

static struct navi_hlist_head *lib_bucket(unsigned long hash)
{
	return &libraries[hash % NAVI_ENV_HT_SIZE];
}

static struct navi_library *register_library(struct navi_library *lib)
{
	if (!libraries_initialized)
		libraries_init();
	navi_hlist_add_head(&lib->chain, lib_bucket(libname_hash(lib->name)));
	return lib;
}

static struct navi_library *find_library(navi_obj name)
{
	if (!libraries_initialized)
		libraries_init();
	struct navi_library *entry;
	struct navi_hlist_head *bucket = lib_bucket(libname_hash(name));
	navi_hlist_for_each_entry(entry, bucket, struct navi_library, chain) {
		if (libname_equal(entry->name, name))
			return entry;
	}
	return NULL;
}

static struct navi_library *make_library(navi_obj name, navi_obj declarations)
{
	struct navi_library *lib = navi_critical_malloc(sizeof(struct navi_library));
	lib->loaded = false;
	lib->name = name;
	lib->declarations = declarations;
	return lib;
}

static bool libname_valid(navi_obj name)
{
	navi_obj cons;
	if (!navi_is_pair(name))
		return false;
	navi_list_for_each(cons, name) {
		navi_obj part = navi_car(cons);
		if (navi_is_symbol(part))
			continue;
		if (!navi_is_num(part))
			return false;
		if (navi_num(part) < 0)
			return false;
	}
	return navi_is_nil(cons);
}

static bool exports_valid(navi_obj exports)
{
	navi_obj cons;
	if (!navi_is_pair(exports))
		return false;
	navi_list_for_each(cons, exports) {
		navi_obj export = navi_car(cons);
		if (navi_is_symbol(export))
			continue;
		if (!navi_is_proper_list(export))
			return false;
		if (navi_list_length(export) != 3)
			return false;
		if (!navi_symbol_eq(navi_car(export), navi_sym_rename))
			return false;
		if (!navi_is_symbol(navi_cadr(export)))
			return false;
		if (!navi_is_symbol(navi_caddr(export)))
			return false;
	}
	return true;
}

static bool import_valid(navi_obj import);

static bool import_only_except_valid(navi_obj args)
{
	if (!navi_is_pair(args))
		return false;
	return import_valid(navi_car(args))
		&& navi_is_list_of(navi_cdr(args), NAVI_SYMBOL, false);
}

static bool import_prefix_valid(navi_obj args)
{
	if (navi_list_length(args) != 2)
		return false;
	return import_valid(navi_car(args)) && navi_is_symbol(navi_cadr(args));
}

static bool import_rename_valid(navi_obj args)
{
	navi_obj cons;
	if (!navi_is_pair(args))
		return false;
	if (!import_valid(navi_car(args)))
		return false;
	navi_list_for_each(cons, navi_cdr(args)) {
		if (!navi_is_list_of(navi_car(cons), NAVI_SYMBOL, false))
			return false;
		if (navi_list_length(navi_car(cons)) != 2)
			return false;
	}
	return true;
}

static bool import_valid(navi_obj import)
{
	navi_obj sym, rest;
	if (navi_is_nil(import) || !navi_is_proper_list(import))
		return false;
	sym = navi_car(import);
	rest = navi_cdr(import);
	if (!navi_is_symbol(sym))
		return false;
	if (sym.p == navi_sym_only.p || sym.p == navi_sym_except.p)
		return import_only_except_valid(rest);
	if (sym.p == navi_sym_prefix.p)
		return import_prefix_valid(rest);
	if (sym.p == navi_sym_rename.p)
		return import_rename_valid(rest);
	return libname_valid(import);
}

static bool imports_valid(navi_obj imports)
{
	navi_obj cons;
	if (!navi_is_pair(imports))
		return false;
	navi_list_for_each(cons, imports) {
		if (!navi_is_proper_list(navi_car(cons)))
			return false;
		if (!import_valid(navi_car(cons)))
			return false;
	}
	return true;
}

static bool declaration_valid(navi_obj declaration, navi_env env)
{
	navi_obj sym = navi_car(declaration);
	navi_obj rest = navi_cdr(declaration);
	if (!navi_is_symbol(sym))
		return false;
	if (!navi_is_pair(rest))
		return false;
	if (sym.p == navi_sym_export.p)
		return exports_valid(rest);
	if (sym.p == navi_sym_import.p)
		return imports_valid(rest);
	if (sym.p == navi_sym_begin.p)
		return true;
	if (sym.p == navi_sym_include.p)
		return false; // TODO
	if (sym.p == navi_sym_include_ci.p)
		return false; // TODO
	if (sym.p == navi_sym_include_libdecl.p)
		return false; // TODO
	if (sym.p == navi_sym_cond_expand.p)
		return false; // TODO
	return false;
}

#define invalid_library_declaration(env, declaration) \
	navi_error(env, "invalid library declaration", \
			navi_make_apair("declaration", declaration))

// FIXME: this approach is inadequate for dealing with
//        include-library-declarations.  What we really need to do is:
//            * do a shallow syntax check
//            * expand include-library-declarations
//            * repeat until there are no more include-library-declarations
//            * do a deep syntax check
static void check_library_syntax(navi_obj name, navi_obj declarations,
		navi_env env)
{
	if (!navi_is_list_of(declarations, NAVI_PROPER_LIST, false))
		navi_error(env, "invalid library definition");
	if (!libname_valid(name))
		navi_error(env, "invalid library name",
				navi_make_apair("name", name));

	navi_obj cons;
	navi_list_for_each(cons, declarations) {
		if (navi_is_nil(navi_car(cons)))
			invalid_library_declaration(env, navi_car(cons));
		if (!declaration_valid(navi_car(cons), env))
			invalid_library_declaration(env, navi_car(cons));
	}
}

static struct navi_library *define_library(navi_obj args, navi_env env)
{
	check_library_syntax(navi_car(args), navi_cdr(args), env);
	return register_library(make_library(navi_car(args), navi_cdr(args)));
}

DEFSPECIAL(define_library, "define-library", 2, NAVI_PROC_VARIADIC,
		NAVI_ANY, NAVI_ANY)
{
	define_library(scm_args, scm_env);
	return navi_unspecified();
}

static const char *navi_lib_path(void)
{
	// TODO: use sane default, implement override mechanism (-L /path/)
	return "";
}

#define LIBNAME_STEP 128

static void write_char(char **path, size_t *length, size_t *wp, char c)
{
	if (wp >= length) {
		*path = navi_critical_realloc(*path, *length + LIBNAME_STEP + 1);
		*length += LIBNAME_STEP;
	}
	(*path)[(*wp)++] = c;
}

static void write_string(char **path, size_t *length, size_t *wp, const char *str)
{
	for (int i = 0; str[i]; i++) {
		char c = str[i];
		if (c == '/')
			c = '_';
		write_char(path, length, wp, c);
	}
}

/*
 * A library name is transformed into a path by concatenating each part of the
 * name, with a path separator between each two parts, and then adding the
 * file extension ".scm" to the result.
 */
static char *libname_to_path(navi_obj name)
{
	navi_obj cons;
	size_t wp = 0;
	size_t length = LIBNAME_STEP;
	char *path = navi_critical_malloc(LIBNAME_STEP+1);
	write_string(&path, &length, &wp, navi_lib_path());
	navi_list_for_each(cons, name) {
		if (!navi_is_symbol(navi_car(cons)))
			continue;
		struct navi_symbol *sym = navi_symbol(navi_car(cons));
		write_string(&path, &length, &wp, sym->data);
		write_char(&path, &length, &wp, '/');
	}
	wp--; // overwrite trailing separator
	write_string(&path, &length, &wp, ".scm");
	path[wp] = '\0';
	return path;
}

/*
 * Read the library given by @name from the filesystem.
 */
static struct navi_library *read_library(navi_obj name, navi_env env)
{
	navi_obj obj;
	char *path = libname_to_path(name);
	struct navi_port *port = navi_port(_navi_open_input_file(path, env));

	free(path);
	obj = navi_read(port, env);
	if (!navi_is_pair(obj) || !navi_is_symbol(navi_car(obj))
			|| navi_car(obj).p != navi_sym_deflib.p)
		navi_error(env, "error reading library",
				navi_make_apair("library", name));

	return define_library(navi_cdr(obj), env);
}

static struct navi_library *do_load_library(struct navi_library *library,
		navi_env env)
{
	navi_obj cons;
	navi_obj declarations = library->declarations;
	navi_obj exports = navi_make_nil();
	navi_env lib_env = new_lexical_environment(env);
	// FIXME: exception during load => reference leak
	navi_list_for_each(cons, declarations) {
		navi_obj sym = navi_caar(cons);
		navi_obj rest = navi_cdar(cons);
		if (sym.p == navi_sym_export.p) {
			exports = navi_list_append_ip(exports, rest);
		} else if (sym.p == navi_sym_import.p) {
			navi_import(rest, lib_env);
		} else if (sym.p == navi_sym_begin.p) {
			navi_force_tail(scm_begin(0, rest, lib_env, NULL), lib_env);
		} else if (sym.p == navi_sym_include.p) {

		} else if (sym.p == navi_sym_include.p) {

		} else if (sym.p == navi_sym_include_ci.p) {

		} else if (sym.p == navi_sym_include_libdecl.p) {

		} else if (sym.p == navi_sym_cond_expand.p) {

		}
	}
	library->loaded = true;
	library->exports = exports;
	library->env = lib_env;
	return library;
}

/*
 * Load the library given by @name, reading the library from the filesystem
 * if it hasn't been defined.
 */
static struct navi_library *load_library(navi_obj name, navi_env env)
{
	struct navi_library *lib = find_library(name);
	if (lib) {
		if (lib->loaded)
			return lib;
		return do_load_library(lib, env);
	}
	lib = read_library(name, env);
	if (!lib)
		return NULL;
	return do_load_library(lib, env);
}

/*
 * Import the object bound to @lib_name in @lib as @export_name in @env.
 */
static void import_binding(struct navi_library *lib, navi_obj lib_name,
		navi_obj export_name, navi_env env)
{
	struct navi_binding *binding = navi_env_binding(lib->env.lexical, lib_name);
	if (!binding)
		navi_error(env, "identifier not found in library",
				navi_make_apair("identifier", lib_name));
	navi_scope_set(env.lexical, export_name, binding->object);
}

/*
 * Import all bindings from @lib into @env, respecting any renames.
 */
static navi_env do_import_library(struct navi_library *lib, navi_env env)
{
	navi_obj cons;
	navi_env import_env = new_lexical_environment(env);
	navi_list_for_each(cons, lib->exports) {
		navi_obj lib_name, export_name, export;
		lib_name = export_name = export = navi_car(cons);
		if (!navi_is_symbol(export)) {
			lib_name = navi_cadr(export);
			export_name = navi_caddr(export);
		}
		import_binding(lib, lib_name, export_name, import_env);
	}
	return import_env;
}

static navi_env import_library(navi_obj name, navi_env env)
{
	struct navi_library *lib = load_library(name, env);
	if (!lib)
		navi_error(env, "no such library",
				navi_make_apair("library", name));
	return do_import_library(lib, env);
}

static navi_env get_import_env(navi_obj set, navi_env env);

static navi_env import_only(navi_obj set, navi_obj includes, navi_env env)
{
	struct navi_binding *binding;
	struct navi_hlist_node *n;
	navi_env import_env = get_import_env(set, env);
	navi_scope_for_each_safe(binding, n, import_env.lexical) {
		navi_obj cons;
		navi_list_for_each(cons, includes) {
			if (navi_car(cons).p == binding->symbol.p)
				goto pass;
		}
		navi_hlist_del(&binding->chain);
pass:
		continue;
	}
	return import_env;
}

static navi_env import_except(navi_obj set, navi_obj excludes, navi_env env)
{
	struct navi_binding *binding;
	struct navi_hlist_node *n;
	navi_env import_env = get_import_env(set, env);
	navi_scope_for_each_safe(binding, n, import_env.lexical) {
		navi_obj cons;
		navi_list_for_each(cons, excludes) {
			if (navi_car(cons).p == binding->symbol.p)
				goto remove;
		}
		continue;
remove:
		navi_hlist_del(&binding->chain);
	}
	return import_env;
}

static navi_obj add_prefix(char *prefix_str, size_t prefix_len, navi_obj symbol)
{
	navi_obj result;
	char *symbol_str = navi_symbol(symbol)->data;
	size_t symbol_len = strlen(symbol_str);
	char *prefixed = navi_critical_malloc(prefix_len + symbol_len + 1);
	memcpy(prefixed, prefix_str, prefix_len);
	memcpy(prefixed+prefix_len, symbol_str, symbol_len);
	prefixed[prefix_len+symbol_len] = '\0';
	result = navi_make_symbol(prefixed);
	free(prefixed);
	return result;
}

static navi_env import_prefix(navi_obj set, navi_obj prefix, navi_env env)
{
	struct navi_binding *binding;
	char *prefix_str = navi_symbol(prefix)->data;
	size_t prefix_len = strlen(prefix_str);
	navi_env import_env = get_import_env(set, env);
	navi_env prefix_env = new_lexical_environment(import_env);
	navi_scope_for_each(binding, import_env.lexical) {
		navi_obj prefixed = add_prefix(prefix_str, prefix_len, binding->symbol);
		navi_scope_set(prefix_env.lexical, prefixed, binding->object);
	}
	navi_env_unref(import_env);
	return prefix_env;
}

static navi_env import_rename(navi_obj set, navi_obj renames, navi_env env)
{
	navi_obj cons;
	navi_env import_env = get_import_env(set, env);
	navi_list_for_each(cons, renames) {
		navi_obj libname = navi_caar(cons);
		navi_obj newname = navi_cadar(cons);
		struct navi_binding *b = navi_scope_lookup(import_env.lexical, libname);
		if (!b) {
			navi_env_unref(import_env);
			navi_error(env, "unbound identifier",
					navi_make_apair("identifier", libname));
		}
		navi_scope_set(import_env.lexical, newname, b->object);
		navi_scope_unset(import_env.lexical, libname);
	}
	return import_env;
}

/*
 * Returns a fresh environment containing bindings for the identifiers given by
 * the import set @set.
 */
static navi_env get_import_env(navi_obj set, navi_env env)
{
	navi_obj sym, rest;
	if (!import_valid(set))
		navi_error(env, "invalid import set");
	sym = navi_car(set), rest = navi_cdr(set);
	if (sym.p == navi_sym_only.p)
		return import_only(navi_car(rest), navi_cdr(rest), env);
	if (sym.p == navi_sym_except.p)
		return import_except(navi_car(rest), navi_cdr(rest), env);
	if (sym.p == navi_sym_prefix.p)
		return import_prefix(navi_car(rest), navi_cadr(rest), env);
	if (sym.p == navi_sym_rename.p)
		return import_rename(navi_car(rest), navi_cdr(rest), env);
	return import_library(set, env);
}

void navi_import(navi_obj imports, navi_env env)
{
	navi_obj cons;
	navi_list_for_each(cons, imports) {
		navi_env import_env = get_import_env(navi_car(cons), env);
		navi_import_all(get_global_scope(env.lexical), import_env.lexical);
		navi_env_unref(import_env);
	}
}

DEFSPECIAL(import, "import", 1, NAVI_PROC_VARIADIC, NAVI_ANY)
{
	if (!imports_valid(scm_args))
		navi_error(scm_env, "invalid import set");
	navi_import(scm_args, scm_env);
	return navi_unspecified();
}
/* Libraries }}} */

static navi_obj sym_pair(const char *str)
{
	return navi_make_pair(navi_make_symbol(str), navi_make_nil());
}

static navi_obj _make_libname(const char *str, ...)
{
	va_list ap;
	navi_obj head, cons;
	va_start(ap, str);
	head = cons = sym_pair(str);
	while ((str = va_arg(ap, const char*))) {
		navi_set_cdr(cons, sym_pair(str));
		cons = navi_cdr(cons);
	}
	return head;
}

#define make_libname(...) \
	_make_libname(__VA_ARGS__, (void*) NULL)

navi_env navi_interaction_environment(void)
{
	navi_obj exn;
	navi_env env = navi_empty_environment();
	navi_obj libs = navi_list(
			make_libname("scheme", "base"),
			make_libname("scheme", "case-lambda"),
			make_libname("scheme", "char"),
			make_libname("scheme", "lazy"),
			make_libname("scheme", "read"),
			make_libname("scheme", "write"));
	navi_import(libs, env);
	exn = navi_from_spec(&SCM_DECL(current_exception_handler), env);
	navi_scope_set(env.lexical, navi_sym_current_exn, exn);
	return env;
}

DEFUN(env_count, "env-count", 0, 0)
{
	unsigned i = 0;
	struct navi_clist_head *it;
	navi_clist_for_each(it, &active_environments) {
		i++;
	}
	printf("nr active environments = %u\n", i);
	return navi_unspecified();
}
