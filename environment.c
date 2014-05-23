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

#include <stdlib.h>
#include <string.h>

#include "sexp.h"

#include "default_bindings.c"

LIST_HEAD(active_environments);

/* FIXME: this is a REALLY bad hash function! */
static unsigned long ptr_hash(sexp_t ptr)
{
	return ptr.n;
}

static struct hlist_head *get_bucket(struct sexp_scope *scope,
		unsigned long hashcode)
{
	return &scope->bindings[hashcode % ENV_HT_SIZE];
}

static struct sexp_binding *make_binding(sexp_t symbol, sexp_t object)
{
	struct sexp_binding *binding = malloc(sizeof(struct sexp_binding));
	binding->symbol = symbol;
	binding->object = object;
	return binding;
}

static struct sexp_binding *scope_lookup(struct sexp_scope *scope,
		sexp_t symbol, unsigned long hashcode)
{
	struct sexp_binding *binding;
	struct hlist_head *hd = get_bucket(scope, hashcode);
	
	hlist_for_each_entry (binding, hd, chain) {
		if (binding->symbol.p == symbol.p)
			return binding;
	}
	return NULL;
}

struct sexp_binding *env_binding(struct sexp_scope *env, sexp_t symbol)
{
	struct sexp_binding *binding;
	unsigned long hashcode = ptr_hash(symbol);

	while (env != NULL) {
		binding = scope_lookup(env, symbol, hashcode);
		if (binding != NULL)
			return binding;
		env = env->next;
	}
	return NULL;
}

static inline struct sexp_scope *make_scope(void)
{
	struct sexp_scope *scope = malloc(sizeof(struct sexp_scope));
	for (unsigned i = 0; i < ENV_HT_SIZE; i++)
		INIT_HLIST_HEAD(&scope->bindings[i]);
	list_add(&scope->chain, &active_environments);
	scope->refs = 1;
	scope->next = NULL;
	return scope;
}

struct sexp_scope *env_new_scope(struct sexp_scope *env)
{
	struct sexp_scope *scope = make_scope();
	scope->next = env;
	scope_ref(env);

	return scope;
}

int env_set(struct sexp_scope *env, sexp_t symbol, sexp_t object)
{
	struct sexp_binding *binding;
	struct hlist_head *head;

	binding = env_binding(env, symbol);
	if (binding != NULL) {
		//sexp_free(binding->object);
		binding->object = object;
		return 1;
	}

	/* FIXME: hash() already computed in env_binding */
	head = get_bucket(env, ptr_hash(symbol));
	binding = make_binding(symbol, object);
	hlist_add_head(&binding->chain, head);
	return 0;
}

int scope_set(struct sexp_scope *env, sexp_t symbol, sexp_t object)
{
	struct sexp_binding *binding;
	unsigned long hashcode = ptr_hash(symbol);

	if ((binding = scope_lookup(env, symbol, hashcode)) != NULL) {
		binding->object = object;
		return 1;
	}

	binding = make_binding(symbol, object);
	hlist_add_head(&binding->chain, get_bucket(env, hashcode));
	return 0;
}

int scope_unset(struct sexp_scope *env, sexp_t symbol)
{
	struct sexp_binding *binding;
	unsigned long hashcode = ptr_hash(symbol);

	if ((binding = scope_lookup(env, symbol, hashcode)) == NULL)
		return 0;

	hlist_del(&binding->chain);
	return 1;
}

struct sexp_scope *extend_environment(struct sexp_scope *env, sexp_t vars,
		sexp_t args)
{
	sexp_t vcons, acons;
	struct sexp_scope *new = env_new_scope(env);

	sexp_zipped_for_each(vcons, acons, vars, args) {
		scope_set(new, car(vcons), car(acons));
	}
	/* dotted tail */
	if (!is_nil(vcons)) {
		scope_set(new, vcons, acons);
	}
	return new;
}

struct sexp_scope *make_default_environment(void)
{
	struct sexp_scope *env = make_scope();

	for (unsigned i = 0; i < NR_DEFAULT_BINDINGS; i++) {
		sexp_t symbol = make_symbol(default_bindings[i].ident);
		sexp_t object = sexp_from_spec(&default_bindings[i]);
		if (sexp_type(object) == SEXP_FUNCTION)
			sexp_fun(object)->env = env;
		env_set(env, symbol, object);
	}

	env_set(env, sym_current_input,  make_stdio_port(stdin));
	env_set(env, sym_current_output, make_stdio_port(stdout));
	env_set(env, sym_current_error,  make_stdio_port(stderr));

	return env;
}

void free_scope(struct sexp_scope *scope)
{
	list_del(&scope->chain);

	for (unsigned i = 0; i < ENV_HT_SIZE; i++) {
		struct sexp_binding *bind;
		struct hlist_node *t;
		hlist_for_each_entry_safe(bind, t, &scope->bindings[i], chain) {
			hlist_del(&bind->chain);
			free(bind);
		}
	}

	if (scope->next != NULL)
		scope_unref(scope->next);
}

DEFUN(scm_env_count, args)
{
	unsigned i = 0;
	struct list_head *it;
	list_for_each(it, &active_environments) {
		i++;
	}
	printf("nr active environments = %u\n", i);
	return unspecified();
}
