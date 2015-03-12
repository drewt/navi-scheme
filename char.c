/* Copyright 2014-2015 Drew Thoreson
 *
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/.
 */

#include <ctype.h>

#include "navi.h"

DEFUN(charp, "char?", 1, 0, NAVI_ANY)
{
	return navi_make_bool(navi_type(scm_arg1) == NAVI_CHAR);
}

#define CHAR_COMPARE(cname, scmname, op) \
	DEFUN(cname, scmname, 2, 0, NAVI_CHAR, NAVI_CHAR) \
	{ \
		navi_obj fst = scm_arg1, snd = scm_arg2; \
		return navi_make_bool(navi_char(fst) op navi_char(snd)); \
	}

CHAR_COMPARE(char_lt,  "char<?",  <)
CHAR_COMPARE(char_gt,  "char>?",  >)
CHAR_COMPARE(char_eq,  "char=?",  ==)
CHAR_COMPARE(char_lte, "char<=?", <=)
CHAR_COMPARE(char_gte, "char>=?", >=)

#define CHAR_CI_COMPARE(cname, scmname, op) \
	DEFUN(cname, scmname, 2, 0, NAVI_CHAR, NAVI_CHAR) \
	{ \
		navi_obj fst = scm_arg1, snd = scm_arg2; \
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

DEFUN(char_upcase, "char-upcase", 1, 0, NAVI_CHAR)
{
	return navi_char_upcase(scm_arg1);
}

DEFUN(char_downcase, "char-downcase", 1, 0, NAVI_CHAR)
{
	return navi_char_downcase(scm_arg1);
}
