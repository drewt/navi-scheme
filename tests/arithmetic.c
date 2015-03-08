/* Copyright 2014-2015 Drew Thoreson
 *
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/.
 */

#include <string.h>
#include "test.h"

/* + */
START_TEST(test_add)
{
	ck_assert_int_eq(navi_fixnum($$(scm_add)), 0);
	ck_assert_int_eq(navi_fixnum($(scm_add, navi_make_fixnum(1))), 1);
	ck_assert_int_eq(navi_fixnum($(scm_add, navi_make_fixnum(1), navi_make_fixnum(2))), 3);
	ck_assert_int_eq(navi_fixnum($(scm_add, navi_make_fixnum(1), navi_make_fixnum(-2))), -1);
}
END_TEST

/* - */
START_TEST(test_sub)
{
	ck_assert_int_eq(navi_fixnum($(scm_sub, navi_make_fixnum(1))), -1);
	ck_assert_int_eq(navi_fixnum($(scm_sub, navi_make_fixnum(2), navi_make_fixnum(1))), 1);
	ck_assert_int_eq(navi_fixnum($(scm_sub, navi_make_fixnum(1), navi_make_fixnum(-1))), 2);
}
END_TEST

/* * */
START_TEST(test_mul)
{
	ck_assert_int_eq(navi_fixnum($$(scm_mul)), 1);
	ck_assert_int_eq(navi_fixnum($(scm_mul, navi_make_fixnum(1))), 1);
	ck_assert_int_eq(navi_fixnum($(scm_mul, navi_make_fixnum(2), navi_make_fixnum(3))), 6);
	ck_assert_int_eq(navi_fixnum($(scm_mul, navi_make_fixnum(2), navi_make_fixnum(-3))), -6);
}
END_TEST

/* / */
START_TEST(test_div)
{
	ck_assert_int_eq(navi_fixnum($(scm_div, navi_make_fixnum(4))), 0);
	ck_assert_int_eq(navi_fixnum($(scm_div, navi_make_fixnum(6), navi_make_fixnum(3))), 2);
	ck_assert_int_eq(navi_fixnum($(scm_div, navi_make_fixnum(6), navi_make_fixnum(-3))), -2);
}
END_TEST

/* quotient */
START_TEST(test_quotient)
{
	ck_assert_int_eq(navi_fixnum($(scm_quotient, navi_make_fixnum(5), navi_make_fixnum(2))), 2);
}
END_TEST

/* remainder */
START_TEST(test_remainder)
{
	ck_assert_int_eq(navi_fixnum($(scm_remainder, navi_make_fixnum(5), navi_make_fixnum(2))), 1);
}
END_TEST

/* < */
START_TEST(test_lt)
{
	ck_assert(navi_bool($$(scm_lt)));
	ck_assert(navi_bool($(scm_lt, navi_make_fixnum(1))));
	ck_assert(navi_bool($(scm_lt, navi_make_fixnum(1), navi_make_fixnum(2))));
	ck_assert(!navi_bool($(scm_lt, navi_make_fixnum(1), navi_make_fixnum(1))));
	ck_assert(!navi_bool($(scm_lt, navi_make_fixnum(2), navi_make_fixnum(1))));
	ck_assert(navi_bool($(scm_lt, navi_make_fixnum(1), navi_make_fixnum(2), navi_make_fixnum(3))));
	ck_assert(!navi_bool($(scm_lt, navi_make_fixnum(1), navi_make_fixnum(2), navi_make_fixnum(2))));
	ck_assert(!navi_bool($(scm_lt, navi_make_fixnum(1), navi_make_fixnum(2), navi_make_fixnum(1))));
}
END_TEST

/* > */
START_TEST(test_gt)
{
	ck_assert(navi_bool($$(scm_gt)));
	ck_assert(navi_bool($(scm_gt, navi_make_fixnum(1))));
	ck_assert(navi_bool($(scm_gt, navi_make_fixnum(2), navi_make_fixnum(1))));
	ck_assert(!navi_bool($(scm_gt, navi_make_fixnum(1), navi_make_fixnum(1))));
	ck_assert(!navi_bool($(scm_gt, navi_make_fixnum(1), navi_make_fixnum(2))));
	ck_assert(navi_bool($(scm_gt, navi_make_fixnum(3), navi_make_fixnum(2), navi_make_fixnum(1))));
	ck_assert(!navi_bool($(scm_gt, navi_make_fixnum(3), navi_make_fixnum(2), navi_make_fixnum(2))));
	ck_assert(!navi_bool($(scm_gt, navi_make_fixnum(3), navi_make_fixnum(2), navi_make_fixnum(3))));
}
END_TEST

/* <= */
START_TEST(test_lte)
{
	ck_assert(navi_bool($$(scm_lte)));
	ck_assert(navi_bool($(scm_lte, navi_make_fixnum(1))));
	ck_assert(navi_bool($(scm_lte, navi_make_fixnum(1), navi_make_fixnum(1))));
	ck_assert(navi_bool($(scm_lte, navi_make_fixnum(1), navi_make_fixnum(2))));
	ck_assert(!navi_bool($(scm_lte, navi_make_fixnum(2), navi_make_fixnum(1))));
	ck_assert(navi_bool($(scm_lte, navi_make_fixnum(1), navi_make_fixnum(2), navi_make_fixnum(3))));
	ck_assert(navi_bool($(scm_lte, navi_make_fixnum(1), navi_make_fixnum(2), navi_make_fixnum(2))));
	ck_assert(!navi_bool($(scm_lte, navi_make_fixnum(1), navi_make_fixnum(2), navi_make_fixnum(1))));
}
END_TEST

/* >= */
START_TEST(test_gte)
{
	ck_assert(navi_bool($$(scm_gte)));
	ck_assert(navi_bool($(scm_gte, navi_make_fixnum(1))));
	ck_assert(navi_bool($(scm_gte, navi_make_fixnum(1), navi_make_fixnum(1))));
	ck_assert(navi_bool($(scm_gte, navi_make_fixnum(2), navi_make_fixnum(1))));
	ck_assert(!navi_bool($(scm_gte, navi_make_fixnum(1), navi_make_fixnum(2))));
	ck_assert(navi_bool($(scm_gte, navi_make_fixnum(3), navi_make_fixnum(2), navi_make_fixnum(1))));
	ck_assert(navi_bool($(scm_gte, navi_make_fixnum(3), navi_make_fixnum(2), navi_make_fixnum(2))));
	ck_assert(!navi_bool($(scm_gte, navi_make_fixnum(3), navi_make_fixnum(2), navi_make_fixnum(3))));
}
END_TEST

/* = */
START_TEST(test_numeq)
{
	ck_assert(navi_bool($$(scm_numeq)));
	ck_assert(navi_bool($(scm_numeq, navi_make_fixnum(1))));
	ck_assert(navi_bool($(scm_numeq, navi_make_fixnum(1), navi_make_fixnum(1))));
	ck_assert(!navi_bool($(scm_numeq, navi_make_fixnum(1), navi_make_fixnum(2))));
	ck_assert(navi_bool($(scm_numeq, navi_make_fixnum(1), navi_make_fixnum(1), navi_make_fixnum(1))));
	ck_assert(!navi_bool($(scm_numeq, navi_make_fixnum(1), navi_make_fixnum(2), navi_make_fixnum(1))));
}
END_TEST

/* zero? */
START_TEST(test_zerop)
{
	ck_assert(navi_bool($(scm_zerop, navi_make_fixnum(0))));
	ck_assert(!navi_bool($(scm_zerop, navi_make_fixnum(1))));
}
END_TEST

/* positive? */
START_TEST(test_positivep)
{
	ck_assert(navi_bool($(scm_positivep, navi_make_fixnum(1))));
	ck_assert(!navi_bool($(scm_positivep, navi_make_fixnum(0))));
	ck_assert(!navi_bool($(scm_positivep, navi_make_fixnum(-1))));
}
END_TEST

/* negative? */
START_TEST(test_negativep)
{
	ck_assert(navi_bool($(scm_negativep, navi_make_fixnum(-1))));
	ck_assert(!navi_bool($(scm_negativep, navi_make_fixnum(0))));
	ck_assert(!navi_bool($(scm_negativep, navi_make_fixnum(1))));
}
END_TEST

/* odd? */
START_TEST(test_oddp)
{
	ck_assert(navi_bool($(scm_oddp, navi_make_fixnum(1))));
	ck_assert(navi_bool($(scm_oddp, navi_make_fixnum(-1))));
	ck_assert(!navi_bool($(scm_oddp, navi_make_fixnum(0))));
	ck_assert(!navi_bool($(scm_oddp, navi_make_fixnum(2))));
}
END_TEST

/* even? */
START_TEST(test_evenp)
{
	ck_assert(navi_bool($(scm_evenp, navi_make_fixnum(0))));
	ck_assert(navi_bool($(scm_evenp, navi_make_fixnum(2))));
	ck_assert(navi_bool($(scm_evenp, navi_make_fixnum(-2))));
	ck_assert(!navi_bool($(scm_evenp, navi_make_fixnum(1))));
}
END_TEST

/* number->string */
START_TEST(test_number_to_string)
{
	navi_obj r;

#define nts_assert(str, ...) \
	r = $(scm_number_to_string, __VA_ARGS__); \
	ck_assert(!strcmp((char*)navi_string(r)->data, str));

	nts_assert("1", navi_make_fixnum(1));
	nts_assert("-1", navi_make_fixnum(-1));
	nts_assert("10", navi_make_fixnum(8), navi_make_fixnum(8));
	nts_assert("10", navi_make_fixnum(10), navi_make_fixnum(10));
	nts_assert("10", navi_make_fixnum(16), navi_make_fixnum(16));
#undef nts_assert
}
END_TEST

/* string->number */
START_TEST(test_string_to_number)
{
	navi_obj r;

#define stn_assert(nr, str) \
	r = $(scm_string_to_number, navi_cstr_to_string(str)); \
	ck_assert_int_eq(navi_fixnum(r), nr);
#define stn_assertb(nr, str, base) \
	r = $(scm_string_to_number, navi_cstr_to_string(str), navi_make_fixnum(base)); \
	ck_assert_int_eq(navi_fixnum(r), nr);

	stn_assert(1,  "1");
	stn_assert(-1, "-1");
	stn_assert(2,  "#b10");
	stn_assert(8,  "#o10");
	stn_assert(10, "#d10");
	stn_assert(16, "#x10");
	stn_assertb(2,  "10", 2);
	stn_assertb(8,  "10", 8);
	stn_assertb(10, "10", 10);
	stn_assertb(16, "10", 16);

#undef stn_assert
#undef stn_assertb
}
END_TEST

/* not */
START_TEST(test_not)
{
	ck_assert(navi_bool($(scm_not, navi_make_bool(false))));
	ck_assert(!navi_bool($(scm_not, navi_make_bool(true))));
}
END_TEST

/* boolean? */
START_TEST(test_booleanp)
{
	ck_assert(navi_bool($(scm_booleanp, navi_make_bool(true))));
	ck_assert(navi_bool($(scm_booleanp, navi_make_bool(false))));
	ck_assert(!navi_bool($(scm_booleanp, navi_make_fixnum(1))));
}
END_TEST

/* boolean=? */
START_TEST(test_boolean_eq)
{
	ck_assert(navi_bool($(scm_boolean_eq, navi_make_bool(true))));
	ck_assert(navi_bool($(scm_boolean_eq, navi_make_bool(false))));
	ck_assert(navi_bool($(scm_boolean_eq, navi_make_bool(true), navi_make_bool(true))));
	ck_assert(!navi_bool($(scm_boolean_eq, navi_make_bool(true), navi_make_bool(false))));
}
END_TEST

TCase *arithmetic_tests(void)
{
	TCase *tc = tcase_create("Core");
	tcase_add_test(tc, test_add);
	tcase_add_test(tc, test_sub);
	tcase_add_test(tc, test_mul);
	tcase_add_test(tc, test_div);
	tcase_add_test(tc, test_quotient);
	tcase_add_test(tc, test_remainder);
	tcase_add_test(tc, test_lt);
	tcase_add_test(tc, test_gt);
	tcase_add_test(tc, test_lte);
	tcase_add_test(tc, test_gte);
	tcase_add_test(tc, test_numeq);
	tcase_add_test(tc, test_zerop);
	tcase_add_test(tc, test_positivep);
	tcase_add_test(tc, test_negativep);
	tcase_add_test(tc, test_oddp);
	tcase_add_test(tc, test_evenp);
	tcase_add_test(tc, test_number_to_string);
	tcase_add_test(tc, test_string_to_number);
	tcase_add_test(tc, test_not);
	tcase_add_test(tc, test_booleanp);
	tcase_add_test(tc, test_boolean_eq);
	return tc;
}
