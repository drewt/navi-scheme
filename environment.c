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
	
	navi_hlist_for_each_entry (binding, hd, chain) {
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

navi_env navi_empty_environment(void)
{
	struct navi_scope *lex = make_scope();
	struct navi_scope *dyn = make_scope();
	if (!lex || !dyn)
		return (navi_env) {0};
	return (navi_env) { .lexical = lex, .dynamic = dyn };
}

navi_env navi_make_environment(const struct navi_spec *bindings[])
{
	navi_env env = navi_empty_environment();
	if (!env.lexical)
		return (navi_env) {0};
	for (unsigned i = 0; bindings[i]; i++) {
		navi_obj symbol = navi_make_symbol(bindings[i]->ident);
		navi_obj object = navi_from_spec(bindings[i], env);
		if (navi_type(object) == NAVI_PROCEDURE)
			navi_procedure(object)->env = env.lexical;
		env_set(env, symbol, object);
	}
	return env;
}

navi_env navi_interaction_environment(void)
{
	return navi_make_environment(default_bindings);
}

void navi_scope_free(struct navi_scope *scope)
{
	navi_clist_del(&scope->chain);

	for (unsigned i = 0; i < NAVI_ENV_HT_SIZE; i++) {
		struct navi_binding *bind;
		struct navi_hlist_node *t;
		navi_hlist_for_each_entry_safe(bind, t, &scope->bindings[i], chain) {
			navi_hlist_del(&bind->chain);
			free(bind);
		}
	}

	if (scope->next != NULL)
		_navi_scope_unref(scope->next);
}

static struct navi_scope *get_global_scope(struct navi_scope *s)
{
	for (; s->next; s = s->next);
	return s;
}

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
	navi_obj result = scm_begin(0, navi_cdr(scm_args), new_env);
	navi_env_unref(new_env);
	return result;
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
