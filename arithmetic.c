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

#define ARITHMETIC_FOLD(operator, args, acc) \
	do { \
		sexp_t ____MAF_CONS; \
		type_check(car(args), SEXP_NUM); \
		acc = car(args); \
		sexp_list_for_each(____MAF_CONS, cdr(args)) { \
			type_check(car(____MAF_CONS), SEXP_NUM); \
			acc.n = operator(acc.n, car(____MAF_CONS).n); \
		} \
	} while (0)

#define ARITHMETIC_DEFUN(cname, operator) \
	DEFUN(cname, ____MAD_ARGS) \
	{ \
		sexp_t ____MAD_ACC; \
		ARITHMETIC_FOLD(operator, ____MAD_ARGS, ____MAD_ACC); \
		return ____MAD_ACC; \
	}

ARITHMETIC_DEFUN(scm_add, fixnum_plus)
ARITHMETIC_DEFUN(scm_sub, fixnum_minus)
ARITHMETIC_DEFUN(scm_mul, fixnum_times)
ARITHMETIC_DEFUN(scm_div, fixnum_divide)

DEFUN(scm_quotient, args)
{
	type_check(car(args),  SEXP_NUM);
	type_check(cadr(args), SEXP_NUM);
	return make_num(sexp_num(car(args)) / sexp_num(cadr(args)));
}

DEFUN(scm_remainder, args)
{
	type_check(car(args),  SEXP_NUM);
	type_check(cadr(args), SEXP_NUM);
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
		_type_check(____MAP_A, SEXP_NUM, ____MAP_ENV); \
		_type_check(____MAP_B, SEXP_NUM, ____MAP_ENV); \
		return sexp_num(____MAP_A) symbol sexp_num(____MAP_B); \
	} \
	DEFUN(cname, args) \
	{ \
		return make_bool(fold_pairs(args, ____ ## cname, ____env)); \
	}

VARIADIC_PREDICATE(scm_lt,    <)
VARIADIC_PREDICATE(scm_gt,    >)
VARIADIC_PREDICATE(scm_lte,   <=)
VARIADIC_PREDICATE(scm_gte,   >=)
VARIADIC_PREDICATE(scm_numeq, ==)

#define UNARY_PREDICATE(cname, test) \
	DEFUN(cname, args) \
	{ \
		type_check(car(args), SEXP_NUM); \
		return make_bool(sexp_num(car(args)) test); \
	}

UNARY_PREDICATE(scm_zerop,     == 0)
UNARY_PREDICATE(scm_positivep, > 0)
UNARY_PREDICATE(scm_negativep, < 0)
UNARY_PREDICATE(scm_oddp,      % 2 != 0)
UNARY_PREDICATE(scm_evenp,     % 2 == 0)

DEFUN(scm_number_to_string, args)
{
	char buf[64];
	long radix = 10;

	type_check(car(args), SEXP_NUM);
	if (sexp_type(cdr(args)) != SEXP_NIL) {
		type_check(cadr(args), SEXP_NUM);
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
		error(____env, "unsupported radix");
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

DEFUN(scm_string_to_number, args)
{
	sexp_t bytes;
	char *string, *endptr;
	long n, radix;

	bytes = type_check(car(args), SEXP_STRING);
	string = (char*) sexp_bytevec(bytes)->data;

	radix = is_nil(cdr(args)) ? fixnum_cast(cadr(args)) : 10;

	if ((n = explicit_radix(string)) != 0) {
		string += 2;
		radix = n;
	}

	n = strtol(string, &endptr, radix);
	if (*endptr != '\0')
		return make_bool(false);
	return make_num(n);
}

DEFUN(scm_not, args)
{
	return make_bool(!sexp_is_true(car(args)));
}

DEFUN(scm_booleanp, args)
{
	return make_bool(sexp_type(car(args)) == SEXP_BOOL);
}

DEFUN(scm_boolean_eq, args)
{
	sexp_t cons;
	sexp_t bval;

	type_check(car(args), SEXP_BOOL);
	bval = car(args);

	sexp_list_for_each(cons, cdr(args)) {
		type_check(car(cons), SEXP_BOOL);
		if (car(cons).n != bval.n)
			return make_bool(false);
	}
	return make_bool(true);
}
