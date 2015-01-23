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

#include <ctype.h>

#include "navi.h"

static inline bool both_chars(sexp_t a, sexp_t b)
{
	return sexp_type(a) == SEXP_CHAR && sexp_type(b) == SEXP_CHAR;
}

DEFUN(scm_charp, args, env)
{
	return sexp_make_bool(sexp_type(car(args)) == SEXP_CHAR);
}

#define BINARY_PREDICATE(cname, op) \
	DEFUN(cname, args, env) \
	{ \
		sexp_t fst = car(args), snd = cadr(args); \
		type_check(fst, SEXP_CHAR, env); \
		type_check(snd, SEXP_CHAR, env); \
		return sexp_make_bool(sexp_char(fst) op sexp_char(snd)); \
	}

BINARY_PREDICATE(scm_char_lt,  <)
BINARY_PREDICATE(scm_char_gt,  >)
BINARY_PREDICATE(scm_char_eq,  ==)
BINARY_PREDICATE(scm_char_lte, <=)
BINARY_PREDICATE(scm_char_gte, >=)

#define BINARY_CI_PREDICATE(cname, op) \
	DEFUN(cname, args, env) \
	{ \
		sexp_t fst = car(args), snd = cadr(args); \
		type_check(fst, SEXP_CHAR, env); \
		type_check(snd, SEXP_CHAR, env); \
		return sexp_make_bool(tolower(sexp_char(fst)) op \
				tolower(sexp_char(snd))); \
	}

BINARY_CI_PREDICATE(scm_char_ci_lt,  <)
BINARY_CI_PREDICATE(scm_char_ci_gt,  >)
BINARY_CI_PREDICATE(scm_char_ci_eq,  ==)
BINARY_CI_PREDICATE(scm_char_ci_lte, <=)
BINARY_CI_PREDICATE(scm_char_ci_gte, >=)

sexp_t char_upcase(sexp_t ch)
{
	return sexp_make_char(toupper(sexp_char(ch)));
}

sexp_t char_downcase(sexp_t ch)
{
	return sexp_make_char(tolower(sexp_char(ch)));
}

DEFUN(scm_char_upcase, args, env)
{
	return char_upcase(type_check(car(args), SEXP_CHAR, env));
}

DEFUN(scm_char_downcase, args, env)
{
	return char_downcase(type_check(car(args), SEXP_CHAR, env));
}
