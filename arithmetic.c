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
		navi_t ____MAF_CONS; \
		navi_type_check(navi_car(args), NAVI_NUM, env); \
		acc = navi_car(args); \
		navi_list_for_each(____MAF_CONS, navi_cdr(args)) { \
			navi_type_check(navi_car(____MAF_CONS), NAVI_NUM, env); \
			acc.n = operator(acc.n, navi_car(____MAF_CONS).n); \
		} \
	} while (0)

#define ARITHMETIC_DEFUN(cname, operator) \
	DEFUN(cname, ____MAD_ARGS, env) \
	{ \
		navi_t ____MAD_ACC; \
		ARITHMETIC_FOLD(operator, ____MAD_ARGS, ____MAD_ACC, env); \
		return ____MAD_ACC; \
	}

DEFUN(scm_add, args, env)
{
	navi_t acc = navi_make_num(0);
	if (navi_is_nil(args))
		return navi_make_num(0);
	if (navi_is_nil(navi_cdr(args)))
		return navi_car(args);
	ARITHMETIC_FOLD(navi_fixnum_plus, args, acc, env);
	return acc;
}

DEFUN(scm_sub, args, env)
{
	navi_t acc;
	if (navi_is_nil(navi_cdr(args))) {
		acc.n = navi_fixnum_minus(navi_make_num(0).n, navi_car(args).n);
		return acc;
	}
	ARITHMETIC_FOLD(navi_fixnum_minus, args, acc, env);
	return acc;
}

DEFUN(scm_mul, args, env)
{
	navi_t acc;
	if (navi_is_nil(args))
		return navi_make_num(1);
	if (navi_is_nil(navi_cdr(args)))
		return navi_car(args);
	ARITHMETIC_FOLD(navi_fixnum_times, args, acc, env);
	return acc;
}

DEFUN(scm_div, args, env)
{
	navi_t acc;
	if (navi_is_nil(navi_cdr(args))) {
		acc.n = navi_fixnum_divide(navi_make_num(1).n, navi_car(args).n);
		return acc;
	}
	ARITHMETIC_FOLD(navi_fixnum_divide, args, acc, env);
	return acc;
}

DEFUN(scm_quotient, args, env)
{
	navi_type_check(navi_car(args),  NAVI_NUM, env);
	navi_type_check(navi_cadr(args), NAVI_NUM, env);
	return navi_make_num(navi_num(navi_car(args)) / navi_num(navi_cadr(args)));
}

DEFUN(scm_remainder, args, env)
{
	navi_type_check(navi_car(args),  NAVI_NUM, env);
	navi_type_check(navi_cadr(args), NAVI_NUM, env);
	return navi_make_num(navi_num(navi_car(args)) % navi_num(navi_cadr(args)));
}

static bool fold_pairs(navi_t list, bool (*compare)(navi_t,navi_t,navi_env_t),
		navi_env_t env)
{
	navi_t cons;

	navi_list_for_each(cons, list) {
		if (navi_type(navi_cdr(cons)) == NAVI_NIL)
			return true;
		if (!compare(navi_car(cons), navi_cadr(cons), env))
			return false;
	}
	return true;
}

#define VARIADIC_PREDICATE(cname, symbol) \
	static bool _ ## cname(navi_t ____MAP_A, navi_t ____MAP_B, \
			navi_env_t ____MAP_ENV) \
	{ \
		navi_type_check(____MAP_A, NAVI_NUM, ____MAP_ENV); \
		navi_type_check(____MAP_B, NAVI_NUM, ____MAP_ENV); \
		return navi_num(____MAP_A) symbol navi_num(____MAP_B); \
	} \
	DEFUN(cname, args, env) \
	{ \
		return navi_make_bool(fold_pairs(args, _ ## cname, env)); \
	}

VARIADIC_PREDICATE(scm_lt,    <)
VARIADIC_PREDICATE(scm_gt,    >)
VARIADIC_PREDICATE(scm_lte,   <=)
VARIADIC_PREDICATE(scm_gte,   >=)
VARIADIC_PREDICATE(scm_numeq, ==)

#define UNARY_PREDICATE(cname, test) \
	DEFUN(cname, args, env) \
	{ \
		navi_type_check(navi_car(args), NAVI_NUM, env); \
		return navi_make_bool(navi_num(navi_car(args)) test); \
	}

UNARY_PREDICATE(scm_zerop,     == 0)
UNARY_PREDICATE(scm_positivep, > 0)
UNARY_PREDICATE(scm_negativep, < 0)
UNARY_PREDICATE(scm_oddp,      % 2 != 0)
UNARY_PREDICATE(scm_evenp,     % 2 == 0)

DEFUN(scm_number_to_string, args, env)
{
	char buf[64];
	long radix = 10;

	navi_type_check(navi_car(args), NAVI_NUM, env);
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

DEFUN(scm_string_to_number, args, env)
{
	navi_t bytes;
	char *string, *endptr;
	long n, radix;

	bytes = navi_type_check(navi_car(args), NAVI_STRING, env);
	string = (char*) navi_string(bytes)->data;

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

DEFUN(scm_not, args, env)
{
	return navi_make_bool(!navi_is_true(navi_car(args)));
}

DEFUN(scm_booleanp, args, env)
{
	return navi_make_bool(navi_type(navi_car(args)) == NAVI_BOOL);
}

DEFUN(scm_boolean_eq, args, env)
{
	navi_t cons;
	navi_t bval;

	navi_type_check(navi_car(args), NAVI_BOOL, env);
	bval = navi_car(args);

	navi_list_for_each(cons, navi_cdr(args)) {
		navi_type_check(navi_car(cons), NAVI_BOOL, env);
		if (navi_car(cons).n != bval.n)
			return navi_make_bool(false);
	}
	return navi_make_bool(true);
}
