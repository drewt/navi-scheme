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

#include "sexp.h"

#define ARITHMETIC_FOLD(operator, args, acc, env) \
	do { \
		sexp_t ____MAF_CONS; \
		type_check(car(args), SEXP_NUM, env); \
		acc = car(args); \
		sexp_list_for_each(____MAF_CONS, cdr(args)) { \
			type_check(car(____MAF_CONS), SEXP_NUM, env); \
			acc.n = operator(acc.n, car(____MAF_CONS).n); \
		} \
	} while (0)

#define ARITHMETIC_DEFUN(cname, operator) \
	DEFUN(cname, ____MAD_ARGS, env) \
	{ \
		sexp_t ____MAD_ACC; \
		ARITHMETIC_FOLD(operator, ____MAD_ARGS, ____MAD_ACC, env); \
		return ____MAD_ACC; \
	}

DEFUN(scm_add, args, env)
{
	sexp_t acc = make_num(0);
	if (is_nil(args))
		return make_num(0);
	if (is_nil(cdr(args)))
		return car(args);
	ARITHMETIC_FOLD(fixnum_plus, args, acc, env);
	return acc;
}

DEFUN(scm_sub, args, env)
{
	sexp_t acc;
	if (is_nil(cdr(args))) {
		acc.n = fixnum_minus(make_num(0).n, car(args).n);
		return acc;
	}
	ARITHMETIC_FOLD(fixnum_minus, args, acc, env);
	return acc;
}

DEFUN(scm_mul, args, env)
{
	sexp_t acc;
	if (is_nil(args))
		return make_num(1);
	if (is_nil(cdr(args)))
		return car(args);
	ARITHMETIC_FOLD(fixnum_times, args, acc, env);
	return acc;
}

DEFUN(scm_div, args, env)
{
	sexp_t acc;
	if (is_nil(cdr(args))) {
		acc.n = fixnum_divide(make_num(1).n, car(args).n);
		return acc;
	}
	ARITHMETIC_FOLD(fixnum_divide, args, acc, env);
	return acc;
}

DEFUN(scm_quotient, args, env)
{
	type_check(car(args),  SEXP_NUM, env);
	type_check(cadr(args), SEXP_NUM, env);
	return make_num(sexp_num(car(args)) / sexp_num(cadr(args)));
}

DEFUN(scm_remainder, args, env)
{
	type_check(car(args),  SEXP_NUM, env);
	type_check(cadr(args), SEXP_NUM, env);
	return make_num(sexp_num(car(args)) % sexp_num(cadr(args)));
}

static bool fold_pairs(sexp_t list, bool (*compare)(sexp_t,sexp_t,env_t),
		env_t env)
{
	sexp_t cons;

	sexp_list_for_each(cons, list) {
		if (sexp_type(cdr(cons)) == SEXP_NIL)
			return true;
		if (!compare(car(cons), cadr(cons), env))
			return false;
	}
	return true;
}

#define VARIADIC_PREDICATE(cname, symbol) \
	static bool ____ ## cname(sexp_t ____MAP_A, sexp_t ____MAP_B, \
			env_t ____MAP_ENV) \
	{ \
		type_check(____MAP_A, SEXP_NUM, ____MAP_ENV); \
		type_check(____MAP_B, SEXP_NUM, ____MAP_ENV); \
		return sexp_num(____MAP_A) symbol sexp_num(____MAP_B); \
	} \
	DEFUN(cname, args, env) \
	{ \
		return make_bool(fold_pairs(args, ____ ## cname, env)); \
	}

VARIADIC_PREDICATE(scm_lt,    <)
VARIADIC_PREDICATE(scm_gt,    >)
VARIADIC_PREDICATE(scm_lte,   <=)
VARIADIC_PREDICATE(scm_gte,   >=)
VARIADIC_PREDICATE(scm_numeq, ==)

#define UNARY_PREDICATE(cname, test) \
	DEFUN(cname, args, env) \
	{ \
		type_check(car(args), SEXP_NUM, env); \
		return make_bool(sexp_num(car(args)) test); \
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

	type_check(car(args), SEXP_NUM, env);
	if (sexp_type(cdr(args)) != SEXP_NIL) {
		type_check(cadr(args), SEXP_NUM, env);
		radix = sexp_num(cadr(args));
	}

	switch (radix) {
	case 8:
		snprintf(buf, 64, "%lo", sexp_num(car(args)));
		break;
	case 10:
		snprintf(buf, 64, "%ld", sexp_num(car(args)));
		break;
	case 16:
		snprintf(buf, 64, "%lx", sexp_num(car(args)));
		break;
	default:
		error(env, "unsupported radix");
	}
	return to_string(buf);
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
	sexp_t bytes;
	char *string, *endptr;
	long n, radix;

	bytes = type_check(car(args), SEXP_STRING, env);
	string = (char*) sexp_bytevec(bytes)->data;

	radix = is_nil(cdr(args)) ? fixnum_cast(cadr(args), env) : 10;

	if ((n = explicit_radix(string)) != 0) {
		string += 2;
		radix = n;
	}

	n = strtol(string, &endptr, radix);
	if (*endptr != '\0')
		return make_bool(false);
	return make_num(n);
}

DEFUN(scm_not, args, env)
{
	return make_bool(!sexp_is_true(car(args)));
}

DEFUN(scm_booleanp, args, env)
{
	return make_bool(sexp_type(car(args)) == SEXP_BOOL);
}

DEFUN(scm_boolean_eq, args, env)
{
	sexp_t cons;
	sexp_t bval;

	type_check(car(args), SEXP_BOOL, env);
	bval = car(args);

	sexp_list_for_each(cons, cdr(args)) {
		type_check(car(cons), SEXP_BOOL, env);
		if (car(cons).n != bval.n)
			return make_bool(false);
	}
	return make_bool(true);
}
