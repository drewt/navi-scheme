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
	struct navi_binding *binding = malloc(sizeof(struct navi_binding));
	if (!binding)
		return NULL;
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

struct navi_binding *navi_env_binding(struct navi_scope *env, navi_obj symbol)
{
	struct navi_binding *binding;
	unsigned long hashcode = ptr_hash(symbol);

	while (env != NULL) {
		binding = scope_lookup(env, symbol, hashcode);
		if (binding != NULL)
			return binding;
		env = env->next;
	}
	return NULL;
}

static inline struct navi_scope *make_scope(void)
{
	struct navi_scope *scope = malloc(sizeof(struct navi_scope));
	if (!scope)
		return NULL;
	for (unsigned i = 0; i < NAVI_ENV_HT_SIZE; i++)
		NAVI_INIT_HLIST_HEAD(&scope->bindings[i]);
	navi_clist_add(&scope->chain, &active_environments);
	scope->refs = 1;
	scope->next = NULL;
	return scope;
}

struct navi_scope *navi_env_new_scope(struct navi_scope *env)
{
	struct navi_scope *scope = make_scope();
	if (!scope)
		return NULL;
	scope->next = env;
	navi_scope_ref(env);
	return scope;
}

static int env_set(struct navi_scope *env, navi_obj symbol, navi_obj object)
{
	struct navi_binding *binding;
	struct navi_hlist_head *head;

	binding = navi_env_binding(env, symbol);
	if (binding) {
		//navi_free(binding->object);
		binding->object = object;
		return 0;
	}

	/* FIXME: hash() already computed in navi_env_binding */
	head = get_bucket(env, ptr_hash(symbol));
	binding = make_binding(symbol, object);
	if (!binding)
		return -1;
	navi_hlist_add_head(&binding->chain, head);
	return 0;
}

static int _navi_scope_set(struct navi_scope *env, navi_obj symbol, navi_obj object)
{
	struct navi_binding *binding;
	unsigned long hashcode = ptr_hash(symbol);

	if ((binding = scope_lookup(env, symbol, hashcode)) != NULL) {
		binding->object = object;
		return 0;
	}

	binding = make_binding(symbol, object);
	if (!binding)
		return -1;
	navi_hlist_add_head(&binding->chain, get_bucket(env, hashcode));
	return 0;
}

/* XXX: assumes we're executing in env */
void navi_scope_set(struct navi_scope *env, navi_obj symbol, navi_obj object)
{
	if (_navi_scope_set(env, symbol, object) < 0)
		navi_enomem(env);
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
struct navi_scope *navi_extend_environment(struct navi_scope *env, navi_obj vars,
		navi_obj args)
{
	navi_obj vcons, acons;
	struct navi_scope *new = navi_env_new_scope(env);

	navi_list_for_each_zipped(vcons, acons, vars, args) {
		navi_scope_set(new, navi_car(vcons), navi_car(acons));
	}
	/* dotted tail */
	if (!navi_is_nil(vcons)) {
		navi_scope_set(new, vcons, acons);
	}
	return new;
}

struct navi_scope *navi_make_environment(const struct navi_spec *bindings[])
{
	struct navi_scope *env = make_scope();
	if (!env)
		return NULL;
	for (unsigned i = 0; i < _NR_DEFAULT_BINDINGS; i++) {
		navi_obj symbol = navi_make_symbol(bindings[i]->ident);
		navi_obj object = navi_from_spec(bindings[i]);
		if (navi_type(object) == NAVI_PROCEDURE)
			navi_procedure(object)->env = env;
		env_set(env, symbol, object);
	}
	return env;
}

struct navi_scope *navi_interaction_environment(void)
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
		navi_scope_unref(scope->next);
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
