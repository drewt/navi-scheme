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

#include <ctype.h>

#include "navi.h"

static inline bool both_chars(navi_t a, navi_t b)
{
	return navi_type(a) == NAVI_CHAR && navi_type(b) == NAVI_CHAR;
}

DEFUN(scm_charp, args, env)
{
	return navi_make_bool(navi_type(navi_car(args)) == NAVI_CHAR);
}

#define BINARY_PREDICATE(cname, op) \
	DEFUN(cname, args, env) \
	{ \
		navi_t fst = navi_car(args), snd = navi_cadr(args); \
		navi_type_check(fst, NAVI_CHAR, env); \
		navi_type_check(snd, NAVI_CHAR, env); \
		return navi_make_bool(navi_char(fst) op navi_char(snd)); \
	}

BINARY_PREDICATE(scm_char_lt,  <)
BINARY_PREDICATE(scm_char_gt,  >)
BINARY_PREDICATE(scm_char_eq,  ==)
BINARY_PREDICATE(scm_char_lte, <=)
BINARY_PREDICATE(scm_char_gte, >=)

#define BINARY_CI_PREDICATE(cname, op) \
	DEFUN(cname, args, env) \
	{ \
		navi_t fst = navi_car(args), snd = navi_cadr(args); \
		navi_type_check(fst, NAVI_CHAR, env); \
		navi_type_check(snd, NAVI_CHAR, env); \
		return navi_make_bool(tolower(navi_char(fst)) op \
				tolower(navi_char(snd))); \
	}

BINARY_CI_PREDICATE(scm_char_ci_lt,  <)
BINARY_CI_PREDICATE(scm_char_ci_gt,  >)
BINARY_CI_PREDICATE(scm_char_ci_eq,  ==)
BINARY_CI_PREDICATE(scm_char_ci_lte, <=)
BINARY_CI_PREDICATE(scm_char_ci_gte, >=)

navi_t navi_char_upcase(navi_t ch)
{
	return navi_make_char(toupper(navi_char(ch)));
}

navi_t navi_char_downcase(navi_t ch)
{
	return navi_make_char(tolower(navi_char(ch)));
}

DEFUN(scm_char_upcase, args, env)
{
	return navi_char_upcase(navi_type_check(navi_car(args), NAVI_CHAR, env));
}

DEFUN(scm_char_downcase, args, env)
{
	return navi_char_downcase(navi_type_check(navi_car(args), NAVI_CHAR, env));
}
