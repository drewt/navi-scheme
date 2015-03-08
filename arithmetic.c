/* Copyright 2014-2015 Drew Thoreson
 *
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/.
 */

#include "navi.h"

#define ARITHMETIC_FOLD(operator, args, acc, env) \
	do { \
		navi_obj ____MAF_CONS; \
		navi_type_check(navi_car(args), NAVI_FIXNUM, env); \
		acc = navi_car(args); \
		navi_list_for_each(____MAF_CONS, navi_cdr(args)) { \
			navi_type_check(navi_car(____MAF_CONS), NAVI_FIXNUM, env); \
			acc.n = operator(acc.n, navi_car(____MAF_CONS).n); \
		} \
	} while (0)

DEFUN(add, "+", 0, NAVI_PROC_VARIADIC)
{
	navi_obj acc = navi_make_fixnum(0);
	if (navi_is_nil(scm_args))
		return navi_make_fixnum(0);
	if (navi_is_nil(navi_cdr(scm_args)))
		return scm_arg1;
	ARITHMETIC_FOLD(navi_fixnum_plus, scm_args, acc, scm_env);
	return acc;
}

DEFUN(sub, "-", 1, NAVI_PROC_VARIADIC, NAVI_FIXNUM)
{
	navi_obj acc;
	if (navi_is_nil(navi_cdr(scm_args))) {
		acc.n = navi_fixnum_minus(navi_make_fixnum(0).n, scm_arg1.n);
		return acc;
	}
	ARITHMETIC_FOLD(navi_fixnum_minus, scm_args, acc, scm_env);
	return acc;
}

DEFUN(mul, "*", 0, NAVI_PROC_VARIADIC)
{
	navi_obj acc;
	if (navi_is_nil(scm_args))
		return navi_make_fixnum(1);
	if (navi_is_nil(navi_cdr(scm_args)))
		return scm_arg1;
	ARITHMETIC_FOLD(navi_fixnum_times, scm_args, acc, scm_env);
	return acc;
}

DEFUN(div, "/", 1, NAVI_PROC_VARIADIC, NAVI_FIXNUM)
{
	navi_obj acc;
	if (navi_is_nil(navi_cdr(scm_args))) {
		acc.n = navi_fixnum_divide(navi_make_fixnum(1).n, scm_arg1.n);
		return acc;
	}
	ARITHMETIC_FOLD(navi_fixnum_divide, scm_args, acc, scm_env);
	return acc;
}

DEFUN(quotient, "quotient", 2, 0, NAVI_FIXNUM, NAVI_FIXNUM)
{
	return navi_make_fixnum(navi_fixnum(scm_arg1) / navi_fixnum(scm_arg2));
}

DEFUN(remainder, "remainder", 2, 0, NAVI_FIXNUM, NAVI_FIXNUM)
{
	return navi_make_fixnum(navi_fixnum(scm_arg1) % navi_fixnum(scm_arg2));
}

static bool fold_pairs(navi_obj list, bool (*compare)(navi_obj,navi_obj,navi_env),
		navi_env env)
{
	navi_obj cons;

	navi_list_for_each(cons, list) {
		if (navi_type(navi_cdr(cons)) == NAVI_NIL)
			return true;
		if (!compare(navi_car(cons), navi_cadr(cons), env))
			return false;
	}
	return true;
}

#define NUMERIC_COMPARISON(cname, scmname, op) \
	static bool _ ## cname(navi_obj ____MAP_A, navi_obj ____MAP_B, \
			navi_env ____MAP_ENV) \
	{ \
		navi_type_check(____MAP_A, NAVI_FIXNUM, ____MAP_ENV); \
		navi_type_check(____MAP_B, NAVI_FIXNUM, ____MAP_ENV); \
		return navi_fixnum(____MAP_A) op navi_fixnum(____MAP_B); \
	} \
	DEFUN(cname, scmname, 1, NAVI_PROC_VARIADIC, NAVI_FIXNUM) \
	{ \
		return navi_make_bool(fold_pairs(scm_args, _ ## cname, scm_env)); \
	}

NUMERIC_COMPARISON(lt,    "<",  <)
NUMERIC_COMPARISON(gt,    ">",  >)
NUMERIC_COMPARISON(lte,   "<=", <=)
NUMERIC_COMPARISON(gte,   ">=", >=)
NUMERIC_COMPARISON(numeq, "=",  ==)

#define NUMERIC_PREDICATE(cname, scmname, test) \
	DEFUN(cname, scmname, 1, 0, NAVI_FIXNUM) \
	{ \
		return navi_make_bool(navi_fixnum(scm_arg1) test); \
	}

NUMERIC_PREDICATE(zerop,     "zero?",     == 0)
NUMERIC_PREDICATE(positivep, "positive?", > 0)
NUMERIC_PREDICATE(negativep, "negative?", < 0)
NUMERIC_PREDICATE(oddp,      "odd?",      % 2 != 0)
NUMERIC_PREDICATE(evenp,     "even?",     % 2 == 0)

DEFUN(number_to_string, "number->string", 1, NAVI_PROC_VARIADIC, NAVI_FIXNUM)
{
	char buf[64];
	long radix = 10;

	if (navi_type(navi_cdr(scm_args)) != NAVI_NIL) {
		navi_type_check(scm_arg2, NAVI_FIXNUM, scm_env);
		radix = navi_fixnum(scm_arg2);
	}

	switch (radix) {
	case 8:
		snprintf(buf, 64, "%lo", navi_fixnum(scm_arg1));
		break;
	case 10:
		snprintf(buf, 64, "%ld", navi_fixnum(scm_arg1));
		break;
	case 16:
		snprintf(buf, 64, "%lx", navi_fixnum(scm_arg1));
		break;
	default:
		navi_error(scm_env, "unsupported radix");
	}
	return navi_cstr_to_string(buf);
}

static int explicit_radix(const char *str)
{
	if (str[0] != '#')
		return 0;
	switch (str[1]) {
	case 'b': return 2;
	case 'o': return 8;
	case 'd': return 10;
	case 'x': return 16;
	}
	return 0;
}

DEFUN(string_to_number, "string->number", 1, NAVI_PROC_VARIADIC, NAVI_STRING)
{
	char *string, *endptr;
	long n, radix;

	string = (char*) navi_string(scm_arg1)->data;
	radix = navi_is_nil(navi_cdr(scm_args)) ? 10 : navi_fixnum_cast(scm_arg2, scm_env);

	if ((n = explicit_radix(string)) != 0) {
		string += 2;
		radix = n;
	}

	n = strtol(string, &endptr, radix);
	if (*endptr != '\0')
		return navi_make_bool(false);
	return navi_make_fixnum(n);
}

DEFUN(not, "not", 1, 0, NAVI_ANY)
{
	return navi_make_bool(!navi_is_true(scm_arg1));
}

DEFUN(booleanp, "boolean?", 1, 0, NAVI_ANY)
{
	return navi_make_bool(navi_type(scm_arg1) == NAVI_BOOL);
}

DEFUN(boolean_eq, "boolean=?", 1, NAVI_PROC_VARIADIC, NAVI_BOOL)
{
	navi_obj cons;
	navi_obj bval;

	bval = scm_arg1;
	navi_list_for_each(cons, navi_cdr(scm_args)) {
		navi_type_check(navi_car(cons), NAVI_BOOL, scm_env);
		if (navi_car(cons).n != bval.n)
			return navi_make_bool(false);
	}
	return navi_make_bool(true);
}
