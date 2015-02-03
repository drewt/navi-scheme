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

#ifndef _NAVI_INTERNAL_H
#define _NAVI_INTERNAL_H

#include "clist.h"
#include "error.h"
#include "macros.h"
#include "symbols.h"

static inline _Noreturn int die(const char *msg, ...)
{
	va_list ap;
	va_start(ap, msg);
	vfprintf(stderr, msg, ap);
	va_end(ap);
	putchar('\n');
	exit(1);
}

static inline void *xmalloc(size_t size)
{
	void *r;
	if (!(r = malloc(size)))
		die("not enough memory");
	return r;
}

static inline void *xrealloc(void *p, size_t size)
{
	void *r;
	if (!(r = realloc(p, size)))
		die("not enough memory");
	return r;
}

navi_t navi_read(struct navi_port *port, navi_env_t env);

#define navi_write(expr, env) scm_write(navi_make_pair(expr, navi_make_nil()), env)
#define navi_display(expr, env) scm_display(navi_make_pair(expr, navi_make_nil()), env)
void _navi_display(struct navi_port *port, navi_t expr, bool write, navi_env_t env);

/* environment.c */
struct navi_binding *navi_env_binding(navi_env_t env, navi_t symbol);
navi_env_t navi_env_new_scope(navi_env_t env);
void navi_scope_set(navi_env_t env, navi_t symbol, navi_t object);
int navi_scope_unset(navi_env_t env, navi_t symbol);
navi_env_t navi_extend_environment(navi_env_t env, navi_t vars, navi_t args);
navi_env_t navi_make_default_environment(void);

static inline navi_t navi_env_lookup(navi_env_t env, navi_t symbol)
{
	struct navi_binding *binding = navi_env_binding(env, symbol);
	return binding == NULL ? navi_make_void() : binding->object;
}

navi_t navi_map(navi_t list, navi_leaf_t fn, void *data);

/* eval.c */
navi_t navi_eval(navi_t expr, navi_env_t env);
navi_t navi_call_escape(navi_t escape, navi_t arg);

navi_t navi_vlist(navi_t first, va_list ap);
navi_t navi_list(navi_t first, ...);
navi_t navi_string_copy(navi_t str);

navi_t navi_capture_env(navi_env_t env);
navi_t navi_from_spec(struct navi_spec *spec);
bool navi_eqvp(navi_t fst, navi_t snd);

static inline bool navi_last_pair(navi_t pair)
{
	return navi_type(navi_cdr(pair)) == NAVI_NIL;
}

static inline bool navi_is_true(navi_t expr)
{
	return navi_type(expr) != NAVI_BOOL || navi_bool(expr);
}

static inline bool navi_symbol_eq(navi_t expr, navi_t symbol)
{
	return navi_is_symbol(expr) && expr.p == symbol.p;
}

static inline int navi_list_length(navi_t list)
{
	int i;

	for (i = 0; navi_type(list) == NAVI_PAIR; i++)
		list = navi_cdr(list);

	if (navi_type(list) != NAVI_NIL)
		die("not a proper list"); // TODO: error

	return i;
}

#endif
