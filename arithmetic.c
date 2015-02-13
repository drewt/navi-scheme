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

#include "navi.h"

#define ARITHMETIC_FOLD(operator, args, acc, env) \
	do { \
		navi_obj ____MAF_CONS; \
		navi_type_check(navi_car(args), NAVI_NUM, env); \
		acc = navi_car(args); \
		navi_list_for_each(____MAF_CONS, navi_cdr(args)) { \
			navi_type_check(navi_car(____MAF_CONS), NAVI_NUM, env); \
			acc.n = operator(acc.n, navi_car(____MAF_CONS).n); \
		} \
	} while (0)

DEFUN(add, args, env, "+", 0, NAVI_PROC_VARIADIC)
{
	navi_obj acc = navi_make_num(0);
	if (navi_is_nil(args))
		return navi_make_num(0);
	if (navi_is_nil(navi_cdr(args)))
		return navi_car(args);
	ARITHMETIC_FOLD(navi_fixnum_plus, args, acc, env);
	return acc;
}

DEFUN(sub, args, env, "-", 1, NAVI_PROC_VARIADIC, NAVI_NUM)
{
	navi_obj acc;
	if (navi_is_nil(navi_cdr(args))) {
		acc.n = navi_fixnum_minus(navi_make_num(0).n, navi_car(args).n);
		return acc;
	}
	ARITHMETIC_FOLD(navi_fixnum_minus, args, acc, env);
	return acc;
}

DEFUN(mul, args, env, "*", 0, NAVI_PROC_VARIADIC)
{
	navi_obj acc;
	if (navi_is_nil(args))
		return navi_make_num(1);
	if (navi_is_nil(navi_cdr(args)))
		return navi_car(args);
	ARITHMETIC_FOLD(navi_fixnum_times, args, acc, env);
	return acc;
}

DEFUN(div, args, env, "/", 1, NAVI_PROC_VARIADIC, NAVI_NUM)
{
	navi_obj acc;
	if (navi_is_nil(navi_cdr(args))) {
		acc.n = navi_fixnum_divide(navi_make_num(1).n, navi_car(args).n);
		return acc;
	}
	ARITHMETIC_FOLD(navi_fixnum_divide, args, acc, env);
	return acc;
}

DEFUN(quotient, args, env, "quotient", 2, 0, NAVI_NUM, NAVI_NUM)
{
	return navi_make_num(navi_num(navi_car(args)) / navi_num(navi_cadr(args)));
}

DEFUN(remainder, args, env, "remainder", 2, 0, NAVI_NUM, NAVI_NUM)
{
	return navi_make_num(navi_num(navi_car(args)) % navi_num(navi_cadr(args)));
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
		navi_type_check(____MAP_A, NAVI_NUM, ____MAP_ENV); \
		navi_type_check(____MAP_B, NAVI_NUM, ____MAP_ENV); \
		return navi_num(____MAP_A) op navi_num(____MAP_B); \
	} \
	DEFUN(cname, args, env, scmname, 1, NAVI_PROC_VARIADIC, NAVI_NUM) \
	{ \
		return navi_make_bool(fold_pairs(args, _ ## cname, env)); \
	}

NUMERIC_COMPARISON(lt,    "<",  <)
NUMERIC_COMPARISON(gt,    ">",  >)
NUMERIC_COMPARISON(lte,   "<=", <=)
NUMERIC_COMPARISON(gte,   ">=", >=)
NUMERIC_COMPARISON(numeq, "=",  ==)

#define NUMERIC_PREDICATE(cname, scmname, test) \
	DEFUN(cname, args, env, scmname, 1, 0, NAVI_NUM) \
	{ \
		navi_type_check(navi_car(args), NAVI_NUM, env); \
		return navi_make_bool(navi_num(navi_car(args)) test); \
	}

NUMERIC_PREDICATE(zerop,     "zero?",     == 0)
NUMERIC_PREDICATE(positivep, "positive?", > 0)
NUMERIC_PREDICATE(negativep, "negative?", < 0)
NUMERIC_PREDICATE(oddp,      "odd?",      % 2 != 0)
NUMERIC_PREDICATE(evenp,     "even?",     % 2 == 0)

DEFUN(number_to_string, args, env, "number->string", 1, NAVI_PROC_VARIADIC,
		NAVI_NUM)
{
	char buf[64];
	long radix = 10;

	if (navi_type(navi_cdr(args)) != NAVI_NIL) {
		navi_type_check(navi_cadr(args), NAVI_NUM, env);
		radix = navi_num(navi_cadr(args));
	}

	switch (radix) {
	case 8:
		snprintf(buf, 64, "%lo", navi_num(navi_car(args)));
		break;
	case 10:
		snprintf(buf, 64, "%ld", navi_num(navi_car(args)));
		break;
	case 16:
		snprintf(buf, 64, "%lx", navi_num(navi_car(args)));
		break;
	default:
		navi_error(env, "unsupported radix");
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

DEFUN(string_to_number, args, env, "string->number", 1, NAVI_PROC_VARIADIC,
		NAVI_STRING)
{
	char *string, *endptr;
	long n, radix;

	string = (char*) navi_string(navi_car(args))->data;
	radix = navi_is_nil(navi_cdr(args)) ? 10 : navi_fixnum_cast(navi_cadr(args), env);

	if ((n = explicit_radix(string)) != 0) {
		string += 2;
		radix = n;
	}

	n = strtol(string, &endptr, radix);
	if (*endptr != '\0')
		return navi_make_bool(false);
	return navi_make_num(n);
}

DEFUN(not, args, env, "not", 1, 0, NAVI_ANY)
{
	return navi_make_bool(!navi_is_true(navi_car(args)));
}

DEFUN(booleanp, args, env, "boolean?", 1, 0, NAVI_ANY)
{
	return navi_make_bool(navi_type(navi_car(args)) == NAVI_BOOL);
}

DEFUN(boolean_eq, args, env, "boolean=?", 1, NAVI_PROC_VARIADIC, NAVI_BOOL)
{
	navi_obj cons;
	navi_obj bval;

	bval = navi_car(args);
	navi_list_for_each(cons, navi_cdr(args)) {
		navi_type_check(navi_car(cons), NAVI_BOOL, env);
		if (navi_car(cons).n != bval.n)
			return navi_make_bool(false);
	}
	return navi_make_bool(true);
}
