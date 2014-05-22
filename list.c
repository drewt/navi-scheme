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

#include <stdarg.h>

#include "sexp.h"

sexp_t vlist(sexp_t first, va_list ap)
{
	sexp_t list, listptr;

	list = listptr = make_empty_pair();
	sexp_pair(list)->car = first;
	sexp_for_each_arg(arg, ap) {
		sexp_pair(listptr)->cdr = make_empty_pair();
		listptr = cdr(listptr);
		sexp_pair(listptr)->car = arg;
	}
	sexp_pair(listptr)->cdr = make_nil();
	return list;
}

sexp_t list(sexp_t first, ...)
{
	va_list ap;
	sexp_t list;

	if (sexp_type(first) == SEXP_VOID)
		return make_nil();

	va_start(ap, first);
	list = vlist(first, ap);
	va_end(ap);

	return list;
}

bool is_proper_list(sexp_t list)
{
	sexp_t cons;
	enum sexp_type type = sexp_type(list);
	if (type == SEXP_NIL)
		return true;
	if (type != SEXP_PAIR)
		return false;
	sexp_list_for_each(cons, list);
	return sexp_type(cons) == SEXP_NIL;
}

sexp_t map(sexp_t sexp, sexp_leaf_t fn, void *data)
{
	sexp_t cons;
	struct sexp_pair head, *ptr;

	ptr = &head;
	sexp_list_for_each(cons, sexp) {
		ptr->cdr = make_empty_pair();
		ptr = sexp_pair(ptr->cdr);
		ptr->car = fn(car(cons), data);
	}
	ptr->cdr = make_nil();
	return head.cdr;
}

DEFUN(scm_cons, args)
{
	return make_pair(car((sexp_t)args), cadr((sexp_t)args));
}

DEFUN(scm_car, args)
{
	type_check(car(args), SEXP_PAIR);
	return car(car(args));
}

DEFUN(scm_cdr, args)
{
	type_check(car(args), SEXP_PAIR);
	return cdr(car(args));
}

DEFUN(scm_pairp, args)
{
	return make_bool(sexp_type(car(args)) == SEXP_PAIR);
}

DEFUN(scm_listp, args)
{
	sexp_t cons, list = car(args);
	enum sexp_type type = sexp_type(list);
	if (type != SEXP_PAIR && type != SEXP_NIL)
		return make_bool(false);
	sexp_list_for_each(cons, list);
	return make_bool(sexp_type(cons) == SEXP_NIL);
}

DEFUN(scm_nullp, args)
{
	return make_bool(sexp_type(car(args)) == SEXP_NIL);
}

DEFUN(scm_length, args)
{
	return make_num(list_length(car(args)));
}

DEFUN(scm_list, args)
{
	return args;
}
