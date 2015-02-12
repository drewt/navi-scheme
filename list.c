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

#include <stdarg.h>

#include "navi.h"

navi_t navi_vlist(navi_t first, va_list ap)
{
	navi_t list, listptr;

	list = listptr = navi_make_empty_pair();
	navi_pair(list)->car = first;
	for (navi_t arg = va_arg(ap, navi_t);
			navi_type(arg) != NAVI_VOID;
			arg = va_arg(ap, navi_t)) {
		navi_pair(listptr)->cdr = navi_make_empty_pair();
		listptr = navi_cdr(listptr);
		navi_pair(listptr)->car = arg;
	}
	navi_pair(listptr)->cdr = navi_make_nil();
	return list;
}

navi_t navi_list(navi_t first, ...)
{
	va_list ap;
	navi_t list;

	if (navi_type(first) == NAVI_VOID)
		return navi_make_nil();

	va_start(ap, first);
	list = navi_vlist(first, ap);
	va_end(ap);

	return list;
}

bool navi_is_proper_list(navi_t list)
{
	navi_t cons;
	enum navi_type type = navi_type(list);
	if (type == NAVI_NIL)
		return true;
	if (type != NAVI_PAIR)
		return false;
	navi_list_for_each(cons, list);
	return navi_is_nil(cons);
}

navi_t navi_map(navi_t list, navi_leaf_t fn, void *data)
{
	navi_t cons;
	struct navi_pair head, *ptr;

	ptr = &head;
	navi_list_for_each(cons, list) {
		ptr->cdr = navi_make_empty_pair();
		ptr = navi_pair(ptr->cdr);
		ptr->car = fn(navi_car(cons), data);
	}
	ptr->cdr = navi_make_nil();
	return head.cdr;
}

DEFUN(scm_cons, args, env)
{
	return navi_make_pair(navi_car((navi_t)args), navi_cadr((navi_t)args));
}

DEFUN(scm_car, args, env)
{
	navi_type_check(navi_car(args), NAVI_PAIR, env);
	return navi_car(navi_car(args));
}

DEFUN(scm_cdr, args, env)
{
	navi_type_check(navi_car(args), NAVI_PAIR, env);
	return navi_cdr(navi_car(args));
}

DEFUN(scm_pairp, args, env)
{
	return navi_make_bool(navi_type(navi_car(args)) == NAVI_PAIR);
}

DEFUN(scm_listp, args, env)
{
	navi_t cons, list = navi_car(args);
	enum navi_type type = navi_type(list);
	if (type != NAVI_PAIR && type != NAVI_NIL)
		return navi_make_bool(false);
	navi_list_for_each(cons, list);
	return navi_make_bool(navi_type(cons) == NAVI_NIL);
}

DEFUN(scm_nullp, args, env)
{
	return navi_make_bool(navi_type(navi_car(args)) == NAVI_NIL);
}

DEFUN(scm_length, args, env)
{
	return navi_make_num(navi_list_length(navi_car(args)));
}

DEFUN(scm_list, args, env)
{
	return args;
}

struct map_apply_arg {
	navi_t proc;
	navi_env_t env;
};

static navi_t map_apply(navi_t elm, void *data)
{
	struct map_apply_arg *arg = data;
	return  navi_eval(navi_make_pair(arg->proc, navi_make_pair(elm, navi_make_nil())),
			arg->env);
}

DEFUN(scm_map, args, env)
{
	struct map_apply_arg arg;

	navi_type_check_proc(navi_car(args), 1, env);
	navi_type_check_list(navi_cadr(args), env);

	arg.proc = navi_car(args);
	arg.env = env;

	return navi_map(navi_cadr(args), map_apply, &arg);
}
