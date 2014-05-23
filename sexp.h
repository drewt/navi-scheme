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

#ifndef _SEXP_H
#define _SEXP_H

#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <stdarg.h>
#include <stdbool.h>

#include "sexp/misc.h"
#include "sexp/types.h"
#include "sexp/clist.h"
#include "sexp/symbols.h"
#include "sexp/lib.h"
#include "sexp/macros.h"
#include "sexp/error.h"

extern struct list_head active_environments;

typedef sexp_t(*sexp_leaf_t)(sexp_t, void*);

sexp_t sexp_read(void);

#define sexp_write(sexp) _display(sexp, true)
#define display(sexp) _display(sexp, false)
void _display(sexp_t sexp, bool write);

/* environment.c */
struct sexp_binding *env_binding(env_t env, sexp_t symbol);
env_t env_new_scope(env_t env);
int env_set(env_t env, sexp_t symbol, sexp_t object);
int scope_set(env_t env, sexp_t symbol, sexp_t object);
int scope_unset(env_t env, sexp_t symbol);
env_t extend_environment(env_t env, sexp_t vars, sexp_t args);
env_t make_default_environment(void);

static inline sexp_t env_lookup(env_t env, sexp_t symbol)
{
	struct sexp_binding *binding = env_binding(env, symbol);
	return binding == NULL ? make_void() : binding->object;
}

sexp_t map(sexp_t sexp, sexp_leaf_t fn, void *data);

/* eval.c */
sexp_t eval(sexp_t sexp, env_t env);
sexp_t eval_begin(sexp_t sexp, env_t env);
sexp_t apply(struct sexp_function *fun, sexp_t args, env_t env);
sexp_t trampoline(sexp_t sexp, env_t env);

sexp_t call_escape(sexp_t escape, sexp_t arg);

sexp_t vlist(sexp_t first, va_list ap);
sexp_t list(sexp_t first, ...);
sexp_t string_copy(sexp_t str);
sexp_t char_upcase(sexp_t ch);
sexp_t char_downcase(sexp_t ch);

static inline bool last_pair(sexp_t pair)
{
	return sexp_type(cdr(pair)) == SEXP_NIL;
}

static inline bool sexp_is_true(sexp_t sexp)
{
	return sexp_type(sexp) != SEXP_BOOL || sexp_bool(sexp);
}

static inline bool symbol_eq(sexp_t sexp, sexp_t symbol)
{
	return is_symbol(sexp) && sexp.p == symbol.p;
}

static inline int list_length(sexp_t list)
{
	int i;

	for (i = 0; sexp_type(list) == SEXP_PAIR; i++)
		list = cdr(list);

	if (sexp_type(list) != SEXP_NIL)
		die("not a proper list"); // TODO: error

	return i;
}

static inline unsigned count_pairs(sexp_t list)
{
	sexp_t cons;
	unsigned i = 0;
	sexp_list_for_each(cons, list) { i++; }
	return i;
}

#endif
