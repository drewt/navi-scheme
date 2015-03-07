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

navi_obj _navi_list(navi_obj first, ...)
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

bool navi_is_type(navi_obj obj, int type)
{
	switch (type) {
	case NAVI_LIST:
		return navi_is_list(obj);
	case NAVI_PROPER_LIST:
		return navi_is_proper_list(obj);
	case NAVI_BYTE:
		return navi_is_byte(obj);
	case NAVI_ANY:
		return true;
	default:
		return navi_type(obj) == (unsigned)type;
	}
}

bool navi_is_list_of(navi_obj list, int type, bool allow_dotted_tail)
{
	navi_obj cons;

	navi_list_for_each(cons, list) {
		if (!navi_is_type(navi_car(cons), type))
			return false;
	}
	if (!allow_dotted_tail && navi_type(cons) != NAVI_NIL)
		return false;
	if (allow_dotted_tail && navi_type(cons) != NAVI_NIL
			&& !navi_is_type(cons, type))
		return false;
	return true;
}

navi_obj navi_list_append_ip(navi_obj a, navi_obj b)
{
	if (navi_is_nil(a))
		return b;
	if (navi_is_nil(b))
		return a;
	navi_set_cdr(navi_last_cons(a), b);
	return a;
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

DEFUN(set_car, "set-car!", 2, 0, NAVI_PAIR, NAVI_ANY)
{
	navi_set_car(scm_arg1, scm_arg2);
	return navi_unspecified();
}

DEFUN(set_cdr, "set-cdr!", 2, 0, NAVI_PAIR, NAVI_ANY)
{
	navi_set_cdr(scm_arg1, scm_arg2);
	return navi_unspecified();
}

DEFUN(pairp, "pair?", 1, 0, NAVI_ANY)
{
	return navi_make_bool(navi_type(scm_arg1) == NAVI_PAIR);
}

DEFUN(nullp, "null?", 1, 0, NAVI_ANY)
{
	return navi_make_bool(navi_type(scm_arg1) == NAVI_NIL);
}

DEFUN(listp, "list?", 1, 0, NAVI_ANY)
{
	return navi_make_bool(navi_is_proper_list(scm_arg1));
}

navi_obj navi_make_list(long k, navi_obj obj)
{
	navi_obj cons = navi_make_nil();
	for (long i = 0; i < k; i++)
		cons = navi_make_pair(obj, cons);
	return cons;
}

DEFUN(make_list, "make-list", 1, NAVI_PROC_VARIADIC, NAVI_FIXNUM)
{
	if (scm_nr_args > 2)
		navi_arity_error(scm_env, navi_make_symbol("make-list"));
	if (navi_fixnum(scm_arg1) < 0)
		navi_error(scm_env, "type error: not a positive integer");
	return navi_make_list(navi_fixnum(scm_arg1),
			scm_nr_args == 2 ? scm_arg2 : navi_make_fixnum(0));
}

DEFUN(length, "length", 1, 0, NAVI_LIST)
{
	int length = navi_list_length_safe(scm_arg1);
	if (length < 0)
		navi_error(scm_env, "type error: not a proper list");
	return navi_make_fixnum(length);
}

DEFUN(list, "list", 0, NAVI_PROC_VARIADIC)
{
	return scm_args;
}

navi_obj navi_list_copy(navi_obj list)
{
	navi_obj copy, cursor, cons;
	if (navi_is_nil(list))
		return navi_make_nil();
	copy = cursor = navi_make_pair(navi_car(list), navi_make_nil());
	navi_list_for_each(cons, navi_cdr(list)) {
		navi_obj pair = navi_make_pair(navi_car(cons), navi_make_nil());
		navi_set_cdr(cursor, pair);
		cursor = navi_cdr(cursor);
	}
	navi_set_cdr(cursor, cons);
	return copy;

}

DEFUN(list_copy, "list-copy", 1, 0, NAVI_LIST)
{
	return navi_list_copy(scm_arg1);
}

DEFUN(append, "append", 1, NAVI_PROC_VARIADIC, NAVI_ANY)
{
	navi_obj cons, first, last;
	first = last = navi_make_pair(navi_make_nil(), navi_make_nil());
	navi_list_for_each(cons, scm_args) {
		if (navi_is_last_pair(cons)) {
			navi_set_cdr(last, navi_car(cons));
			break;
		}
		navi_type_check_proper_list(navi_car(cons), scm_env);
		navi_set_cdr(last, navi_list_copy(navi_car(cons)));
		last = navi_last_cons(last);
	}
	return navi_cdr(first);
}

DEFUN(reverse, "reverse", 1, 0, NAVI_LIST)
{
	navi_obj cons, reversed = navi_make_nil();
	navi_list_for_each(cons, scm_arg1) {
		reversed = navi_make_pair(navi_car(cons), reversed);
	}
	// XXX: we ignore dotted tail on improper list
	return reversed;
}

navi_obj navi_list_tail(navi_obj list, long k)
{
	navi_obj cons;
	long i = 0;
	navi_list_for_each(cons, list) {
		if (i++ == k)
			return cons;
	}
	if (i == k)
		return cons;
	return navi_make_void();
}

DEFUN(list_tail, "list-tail", 2, 0, NAVI_LIST, NAVI_FIXNUM)
{
	navi_obj result;
	if (navi_fixnum(scm_arg2) < 0)
		navi_error(scm_env, "type error: not a positive integer");
	result = navi_list_tail(scm_arg1, navi_fixnum(scm_arg2));
	if (navi_is_void(result))
		navi_error(scm_env, "list index out of bounds");
	return result;
}

navi_obj navi_list_ref(navi_obj list, long k)
{
	navi_obj tail = navi_list_tail(list, k);
	if (!navi_is_pair(tail))
		return navi_make_void();
	return navi_car(tail);
}

DEFUN(list_ref, "list-ref", 2, 0, NAVI_PAIR, NAVI_FIXNUM)
{
	navi_obj result;
	if (navi_fixnum(scm_arg2) < 0)
		navi_error(scm_env, "type error: not a positive integer");
	result = navi_list_tail(scm_arg1, navi_fixnum(scm_arg2));
	if (!navi_is_pair(result))
		navi_error(scm_env, "list index out of bounds");
	return navi_car(result);
}

DEFUN(list_set, "list-set!", 3, 0, NAVI_PAIR, NAVI_FIXNUM, NAVI_ANY)
{
	navi_obj result;
	if (navi_fixnum(scm_arg2) < 0)
		navi_error(scm_env, "type error: not a positive integer");
	result = navi_list_tail(scm_arg1, navi_fixnum(scm_arg2));
	if (!navi_is_pair(result))
		navi_error(scm_env, "list index out of bounds");
	navi_set_car(result, scm_arg3);
	return navi_unspecified();
}

DEFUN(memq, "memq", 2, 0, NAVI_ANY, NAVI_LIST)
{
	navi_obj cons;
	navi_list_for_each(cons, scm_arg2) {
		if (navi_eqp(scm_arg1, navi_car(cons)))
			return cons;
	}
	return navi_make_bool(false);
}

DEFUN(memv, "memv", 2, 0, NAVI_ANY, NAVI_LIST)
{
	navi_obj cons;
	navi_list_for_each(cons, scm_arg2) {
		if (navi_eqvp(scm_arg1, navi_car(cons)))
			return cons;
	}
	return navi_make_bool(false);
}

static navi_obj navi_member(navi_obj obj, navi_obj list)
{
	navi_obj cons;
	navi_list_for_each(cons, list) {
		if (navi_equalp(obj, navi_car(cons)))
			return cons;
	}
	return navi_make_bool(false);
}

static bool do_compare(navi_obj proc, navi_obj fst, navi_obj snd, navi_env env)
{
	return navi_is_true(navi_force_tail(navi_apply(navi_procedure(proc),
					navi_list(fst, snd, navi_make_void()), env), env));
}

DEFUN(member, "member", 2, NAVI_PROC_VARIADIC, NAVI_ANY, NAVI_LIST)
{
	navi_obj cons;
	if (scm_nr_args > 3)
		navi_arity_error(scm_env, navi_make_symbol("member"));
	if (scm_nr_args == 2)
		return navi_member(scm_arg1, scm_arg2);
	navi_type_check_proc(scm_arg3, 2, scm_env);
	navi_list_for_each(cons, scm_arg2) {
		if (do_compare(scm_arg3, scm_arg1, navi_car(cons), scm_env))
			return cons;
	}
	return navi_make_bool(false);
}

DEFUN(assq, "assq", 2, 0, NAVI_ANY, NAVI_LIST)
{
	navi_obj cons;
	navi_list_for_each(cons, scm_arg2) {
		navi_type_check(navi_car(cons), NAVI_PAIR, scm_env);
		if (navi_eqp(scm_arg1, navi_caar(cons)))
			return navi_car(cons);
	}
	return navi_make_bool(false);
}

DEFUN(assv, "assv", 2, 0, NAVI_ANY, NAVI_LIST)
{
	navi_obj cons;
	navi_list_for_each(cons, scm_arg2) {
		navi_type_check(navi_car(cons), NAVI_PAIR, scm_env);
		if (navi_eqvp(scm_arg1, navi_caar(cons)))
			return navi_car(cons);
	}
	return navi_make_bool(false);
}

static navi_obj navi_assoc(navi_obj obj, navi_obj list, navi_env env)
{
	navi_obj cons;
	navi_list_for_each(cons, list) {
		navi_type_check(navi_car(cons), NAVI_PAIR, env);
		if (navi_equalp(obj, navi_caar(cons)))
			return navi_car(cons);
	}
	return navi_make_bool(false);
}

DEFUN(assoc, "assoc", 2, NAVI_PROC_VARIADIC, NAVI_ANY, NAVI_LIST)
{
	navi_obj cons;
	if (scm_nr_args > 3)
		navi_arity_error(scm_env, navi_make_symbol("assoc"));
	if (scm_nr_args == 2)
		return navi_assoc(scm_arg1, scm_arg2, scm_env);
	navi_list_for_each(cons, scm_arg2) {
		navi_type_check(navi_car(cons), NAVI_PAIR, scm_env);
		if (do_compare(scm_arg3, scm_arg1, navi_caar(cons), scm_env))
			return navi_car(cons);
	}
	return navi_make_bool(false);
}

static navi_obj arg_list(navi_obj *lists, int nr_lists)
{
	navi_obj args, cons;
	cons = args = navi_make_pair(navi_make_nil(), navi_make_nil());
	for (int i = 0; i < nr_lists; i++) {
		navi_obj cdr = navi_make_pair(navi_car(lists[i]), navi_make_nil());
		navi_set_cdr(cons, cdr);
		cons = navi_cdr(cons);
	}
	return navi_cdr(args);
}

static navi_obj do_apply(navi_obj proc, navi_obj args, navi_env env)
{
	return navi_force_tail(navi_apply(navi_procedure(proc), args, env), env);
}

static void cons_array_fill(navi_obj *array, navi_obj args, navi_env env)
{
	navi_obj cons;
	int i = 0;
	navi_list_for_each(cons, args) {
		array[i] = navi_car(cons);
		navi_type_check_list(array[i++], env);
	}
}

static bool cons_array_terminated(navi_obj *array, size_t nr_lists)
{
	for (size_t i = 0; i < nr_lists; i++)
		if (!navi_is_pair(array[i]))
			return true;
	return false;
}

static void cons_array_advance(navi_obj *array, size_t nr_lists)
{
	for (size_t i = 0; i < nr_lists; i++)
		array[i] = navi_cdr(array[i]);
}

DEFUN(for_each, "for-each", 2, NAVI_PROC_VARIADIC, NAVI_PROCEDURE, NAVI_ANY)
{
	navi_obj cons[scm_nr_args-1];
	navi_check_arity(scm_arg1, scm_nr_args-1, scm_env);
	cons_array_fill(cons, navi_cdr(scm_args), scm_env);
	for (;;) {
		if (cons_array_terminated(cons, scm_nr_args-1))
			break;
		do_apply(scm_arg1, arg_list(cons, scm_nr_args-1), scm_env);
		cons_array_advance(cons, scm_nr_args-1);
	}
	return navi_unspecified();
}

DEFUN(map, "map", 2, NAVI_PROC_VARIADIC, NAVI_PROCEDURE, NAVI_ANY)
{
	navi_obj result, last;
	navi_obj cons[scm_nr_args-1];
	navi_check_arity(scm_arg1, scm_nr_args-1, scm_env);
	cons_array_fill(cons, navi_cdr(scm_args), scm_env);
	result = last = navi_make_pair(navi_make_nil(), navi_make_nil());
	for (;;) {
		navi_obj elm;
		if (cons_array_terminated(cons, scm_nr_args-1))
			break;
		elm = do_apply(scm_arg1, arg_list(cons, scm_nr_args-1), scm_env);
		navi_set_cdr(last, navi_make_pair(elm, navi_make_nil()));
		last = navi_cdr(last);
		cons_array_advance(cons, scm_nr_args-1);
	}
	return navi_cdr(result);
}
