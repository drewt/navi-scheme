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

static inline bool both_chars(navi_obj a, navi_obj b)
{
	return navi_type(a) == NAVI_CHAR && navi_type(b) == NAVI_CHAR;
}

DEFUN(charp, args, env, "char?", 1, 0, NAVI_ANY)
{
	return navi_make_bool(navi_type(navi_car(args)) == NAVI_CHAR);
}

#define CHAR_COMPARE(cname, scmname, op) \
	DEFUN(cname, args, env, scmname, 2, 0, NAVI_CHAR, NAVI_CHAR) \
	{ \
		navi_obj fst = navi_car(args), snd = navi_cadr(args); \
		return navi_make_bool(navi_char(fst) op navi_char(snd)); \
	}

CHAR_COMPARE(char_lt,  "char<?",  <)
CHAR_COMPARE(char_gt,  "char>?",  >)
CHAR_COMPARE(char_eq,  "char=?",  ==)
CHAR_COMPARE(char_lte, "char<=?", <=)
CHAR_COMPARE(char_gte, "char>=?", >=)

#define CHAR_CI_COMPARE(cname, scmname, op) \
	DEFUN(cname, args, env, scmname, 2, 0, NAVI_CHAR, NAVI_CHAR) \
	{ \
		navi_obj fst = navi_car(args), snd = navi_cadr(args); \
		return navi_make_bool(tolower(navi_char(fst)) op \
				tolower(navi_char(snd))); \
	}

CHAR_CI_COMPARE(char_ci_lt,  "char-ci<?",  <)
CHAR_CI_COMPARE(char_ci_gt,  "char-ci>?",  >)
CHAR_CI_COMPARE(char_ci_eq,  "char-ci=?",  ==)
CHAR_CI_COMPARE(char_ci_lte, "char-ci<=?", <=)
CHAR_CI_COMPARE(char_ci_gte, "char-ci>=?", >=)

navi_obj navi_char_upcase(navi_obj ch)
{
	return navi_make_char(toupper(navi_char(ch)));
}

navi_obj navi_char_downcase(navi_obj ch)
{
	return navi_make_char(tolower(navi_char(ch)));
}

DEFUN(char_upcase, args, env, "char-upcase", 1, 0, NAVI_CHAR)
{
	return navi_char_upcase(navi_car(args));
}

DEFUN(char_downcase, args, env, "char-downcase", 1, 0, NAVI_CHAR)
{
	return navi_char_downcase(navi_car(args));
}
