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

navi_obj navi_vlist(navi_obj first, va_list ap)
{
	navi_obj list, listptr;

	list = listptr = navi_make_empty_pair();
	navi_pair(list)->car = first;
	for (navi_obj arg = va_arg(ap, navi_obj);
			navi_type(arg) != NAVI_VOID;
			arg = va_arg(ap, navi_obj)) {
		navi_pair(listptr)->cdr = navi_make_empty_pair();
		listptr = navi_cdr(listptr);
		navi_pair(listptr)->car = arg;
	}
	navi_pair(listptr)->cdr = navi_make_nil();
	return list;
}

navi_obj navi_list(navi_obj first, ...)
{
	va_list ap;
	navi_obj list;

	if (navi_type(first) == NAVI_VOID)
		return navi_make_nil();

	va_start(ap, first);
	list = navi_vlist(first, ap);
	va_end(ap);

	return list;
}

int navi_list_length(navi_obj list)
{
	int i;

	for (i = 0; navi_type(list) == NAVI_PAIR; i++)
		list = navi_cdr(list);

	if (navi_type(list) != NAVI_NIL)
		navi_die("navi_list_length: not a proper list");

	return i;
}

int navi_list_length_safe(navi_obj list)
{
	int i;
	for (i = 0; navi_is_pair(list); i++)
		list = navi_cdr(list);
	if (navi_type(list) != NAVI_NIL)
		return -1;
	return i;
}

bool navi_is_proper_list(navi_obj list)
{
	navi_obj cons;
	enum navi_type type = navi_type(list);
	if (type == NAVI_NIL)
		return true;
	if (type != NAVI_PAIR)
		return false;
	navi_list_for_each(cons, list);
	return navi_is_nil(cons);
}

navi_obj navi_map(navi_obj list, navi_leaf fn, void *data)
{
	navi_obj cons;
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

DEFUN(cons, "cons", 2, 0, NAVI_ANY, NAVI_ANY)
{
	return navi_make_pair(scm_arg1, scm_arg2);
}

DEFUN(car, "car", 1, 0, NAVI_PAIR)
{
	return navi_car(scm_arg1);
}

DEFUN(cdr, "cdr", 1, 0, NAVI_PAIR)
{
	return navi_cdr(scm_arg1);
}

DEFUN(pairp, "pair?", 1, 0, NAVI_ANY)
{
	return navi_make_bool(navi_type(scm_arg1) == NAVI_PAIR);
}

DEFUN(listp, "list?", 1, 0, NAVI_ANY)
{
	navi_obj cons, list = scm_arg1;
	enum navi_type type = navi_type(list);
	if (type != NAVI_PAIR && type != NAVI_NIL)
		return navi_make_bool(false);
	navi_list_for_each(cons, list);
	return navi_make_bool(navi_type(cons) == NAVI_NIL);
}

DEFUN(nullp, "null?", 1, 0, NAVI_ANY)
{
	return navi_make_bool(navi_type(scm_arg1) == NAVI_NIL);
}

DEFUN(length, "length", 1, 0, NAVI_LIST)
{
	return navi_make_num(navi_list_length(scm_arg1));
}

DEFUN(list, "list", 0, NAVI_PROC_VARIADIC)
{
	return scm_args;
}

struct map_apply_arg {
	navi_obj proc;
	navi_env env;
};

static navi_obj map_apply(navi_obj elm, void *data)
{
	struct map_apply_arg *arg = data;
	struct navi_procedure *p = navi_procedure(arg->proc);
	navi_obj args = navi_make_pair(elm, navi_make_nil());
	return navi_force_tail(navi_apply(p, args, arg->env), arg->env);
}

DEFUN(map, "map", 2, 0, NAVI_PROCEDURE, NAVI_LIST)
{
	struct map_apply_arg arg;
	navi_check_arity(scm_arg1, 1, scm_env);
	arg.proc = scm_arg1;
	arg.env = scm_env;
	return navi_map(scm_arg2, map_apply, &arg);
}
