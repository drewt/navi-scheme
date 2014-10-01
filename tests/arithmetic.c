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

#include <string.h>
#include "test.h"

/* + */
START_TEST(test_add)
{
	ck_assert_int_eq(sexp_num(CALL0(scm_add)), 0);
	ck_assert_int_eq(sexp_num(CALL(scm_add, make_num(1))), 1);
	ck_assert_int_eq(sexp_num(CALL(scm_add, make_num(1), make_num(2))), 3);
	ck_assert_int_eq(sexp_num(CALL(scm_add, make_num(1), make_num(-2))), -1);
}
END_TEST

/* - */
START_TEST(test_sub)
{
	ck_assert_int_eq(sexp_num(CALL(scm_sub, make_num(1))), -1);
	ck_assert_int_eq(sexp_num(CALL(scm_sub, make_num(2), make_num(1))), 1);
	ck_assert_int_eq(sexp_num(CALL(scm_sub, make_num(1), make_num(-1))), 2);
}
END_TEST

/* * */
START_TEST(test_mul)
{
	ck_assert_int_eq(sexp_num(CALL0(scm_mul)), 1);
	ck_assert_int_eq(sexp_num(CALL(scm_mul, make_num(1))), 1);
	ck_assert_int_eq(sexp_num(CALL(scm_mul, make_num(2), make_num(3))), 6);
	ck_assert_int_eq(sexp_num(CALL(scm_mul, make_num(2), make_num(-3))), -6);
}
END_TEST

/* / */
START_TEST(test_div)
{
	ck_assert_int_eq(sexp_num(CALL(scm_div, make_num(4))), 0);
	ck_assert_int_eq(sexp_num(CALL(scm_div, make_num(6), make_num(3))), 2);
	ck_assert_int_eq(sexp_num(CALL(scm_div, make_num(6), make_num(-3))), -2);
}
END_TEST

/* quotient */
START_TEST(test_quotient)
{
	ck_assert_int_eq(sexp_num(CALL(scm_quotient, make_num(5), make_num(2))), 2);
}
END_TEST

/* remainder */
START_TEST(test_remainder)
{
	ck_assert_int_eq(sexp_num(CALL(scm_remainder, make_num(5), make_num(2))), 1);
}
END_TEST

/* < */
START_TEST(test_lt)
{
	ck_assert(sexp_bool(CALL0(scm_lt)));
	ck_assert(sexp_bool(CALL(scm_lt, make_num(1))));
	ck_assert(sexp_bool(CALL(scm_lt, make_num(1), make_num(2))));
	ck_assert(!sexp_bool(CALL(scm_lt, make_num(1), make_num(1))));
	ck_assert(!sexp_bool(CALL(scm_lt, make_num(2), make_num(1))));
	ck_assert(sexp_bool(CALL(scm_lt, make_num(1), make_num(2), make_num(3))));
	ck_assert(!sexp_bool(CALL(scm_lt, make_num(1), make_num(2), make_num(2))));
	ck_assert(!sexp_bool(CALL(scm_lt, make_num(1), make_num(2), make_num(1))));
}
END_TEST

/* > */
START_TEST(test_gt)
{
	ck_assert(sexp_bool(CALL0(scm_gt)));
	ck_assert(sexp_bool(CALL(scm_gt, make_num(1))));
	ck_assert(sexp_bool(CALL(scm_gt, make_num(2), make_num(1))));
	ck_assert(!sexp_bool(CALL(scm_gt, make_num(1), make_num(1))));
	ck_assert(!sexp_bool(CALL(scm_gt, make_num(1), make_num(2))));
	ck_assert(sexp_bool(CALL(scm_gt, make_num(3), make_num(2), make_num(1))));
	ck_assert(!sexp_bool(CALL(scm_gt, make_num(3), make_num(2), make_num(2))));
	ck_assert(!sexp_bool(CALL(scm_gt, make_num(3), make_num(2), make_num(3))));
}
END_TEST

/* <= */
START_TEST(test_lte)
{
	ck_assert(sexp_bool(CALL0(scm_lte)));
	ck_assert(sexp_bool(CALL(scm_lte, make_num(1))));
	ck_assert(sexp_bool(CALL(scm_lte, make_num(1), make_num(1))));
	ck_assert(sexp_bool(CALL(scm_lte, make_num(1), make_num(2))));
	ck_assert(!sexp_bool(CALL(scm_lte, make_num(2), make_num(1))));
	ck_assert(sexp_bool(CALL(scm_lte, make_num(1), make_num(2), make_num(3))));
	ck_assert(sexp_bool(CALL(scm_lte, make_num(1), make_num(2), make_num(2))));
	ck_assert(!sexp_bool(CALL(scm_lte, make_num(1), make_num(2), make_num(1))));
}
END_TEST

/* >= */
START_TEST(test_gte)
{
	ck_assert(sexp_bool(CALL0(scm_gte)));
	ck_assert(sexp_bool(CALL(scm_gte, make_num(1))));
	ck_assert(sexp_bool(CALL(scm_gte, make_num(1), make_num(1))));
	ck_assert(sexp_bool(CALL(scm_gte, make_num(2), make_num(1))));
	ck_assert(!sexp_bool(CALL(scm_gte, make_num(1), make_num(2))));
	ck_assert(sexp_bool(CALL(scm_gte, make_num(3), make_num(2), make_num(1))));
	ck_assert(sexp_bool(CALL(scm_gte, make_num(3), make_num(2), make_num(2))));
	ck_assert(!sexp_bool(CALL(scm_gte, make_num(3), make_num(2), make_num(3))));
}
END_TEST

/* = */
START_TEST(test_numeq)
{
	ck_assert(sexp_bool(CALL0(scm_numeq)));
	ck_assert(sexp_bool(CALL(scm_numeq, make_num(1))));
	ck_assert(sexp_bool(CALL(scm_numeq, make_num(1), make_num(1))));
	ck_assert(!sexp_bool(CALL(scm_numeq, make_num(1), make_num(2))));
	ck_assert(sexp_bool(CALL(scm_numeq, make_num(1), make_num(1), make_num(1))));
	ck_assert(!sexp_bool(CALL(scm_numeq, make_num(1), make_num(2), make_num(1))));
}
END_TEST

/* zero? */
START_TEST(test_zerop)
{
	ck_assert(sexp_bool(CALL(scm_zerop, make_num(0))));
	ck_assert(!sexp_bool(CALL(scm_zerop, make_num(1))));
}
END_TEST

/* positive? */
START_TEST(test_positivep)
{
	ck_assert(sexp_bool(CALL(scm_positivep, make_num(1))));
	ck_assert(!sexp_bool(CALL(scm_positivep, make_num(0))));
	ck_assert(!sexp_bool(CALL(scm_positivep, make_num(-1))));
}
END_TEST

/* negative? */
START_TEST(test_negativep)
{
	ck_assert(sexp_bool(CALL(scm_negativep, make_num(-1))));
	ck_assert(!sexp_bool(CALL(scm_negativep, make_num(0))));
	ck_assert(!sexp_bool(CALL(scm_negativep, make_num(1))));
}
END_TEST

/* odd? */
START_TEST(test_oddp)
{
	ck_assert(sexp_bool(CALL(scm_oddp, make_num(1))));
	ck_assert(sexp_bool(CALL(scm_oddp, make_num(-1))));
	ck_assert(!sexp_bool(CALL(scm_oddp, make_num(0))));
	ck_assert(!sexp_bool(CALL(scm_oddp, make_num(2))));
}
END_TEST

/* even? */
START_TEST(test_evenp)
{
	ck_assert(sexp_bool(CALL(scm_evenp, make_num(0))));
	ck_assert(sexp_bool(CALL(scm_evenp, make_num(2))));
	ck_assert(sexp_bool(CALL(scm_evenp, make_num(-2))));
	ck_assert(!sexp_bool(CALL(scm_evenp, make_num(1))));
}
END_TEST

/* number->string */
START_TEST(test_number_to_string)
{
	sexp_t r;

#define nts_assert(str, ...) \
	r = CALL(scm_number_to_string, __VA_ARGS__); \
	ck_assert(!strcmp(sexp_string(r)->data, str));

	nts_assert("1", make_num(1));
	nts_assert("-1", make_num(-1));
	nts_assert("10", make_num(8), make_num(8));
	nts_assert("10", make_num(10), make_num(10));
	nts_assert("10", make_num(16), make_num(16));
#undef nts_assert
}
END_TEST

/* string->number */
START_TEST(test_string_to_number)
{
	sexp_t r;

#define stn_assert(nr, str, ...) \
	r = CALL(scm_string_to_number, to_string(str), ## __VA_ARGS__); \
	ck_assert_int_eq(sexp_num(r), nr);

	stn_assert(1,  "1");
	stn_assert(-1, "-1");
	stn_assert(2,  "#b10");
	stn_assert(8,  "#o10");
	stn_assert(10, "#d10");
	stn_assert(16, "#x10");
	stn_assert(2,  "10", make_num(2));
	stn_assert(8,  "10", make_num(8));
	stn_assert(10, "10", make_num(10));
	stn_assert(16, "10", make_num(16));
#undef stn_assert
}
END_TEST

/* not */
START_TEST(test_not)
{
	ck_assert(sexp_bool(CALL(scm_not, make_bool(false))));
	ck_assert(!sexp_bool(CALL(scm_not, make_bool(true))));
}
END_TEST

/* boolean? */
START_TEST(test_booleanp)
{
	ck_assert(sexp_bool(CALL(scm_booleanp, make_bool(true))));
	ck_assert(sexp_bool(CALL(scm_booleanp, make_bool(false))));
	ck_assert(!sexp_bool(CALL(scm_booleanp, make_num(1))));
}
END_TEST

/* boolean=? */
START_TEST(test_boolean_eq)
{
	ck_assert(sexp_bool(CALL(scm_boolean_eq, make_bool(true))));
	ck_assert(sexp_bool(CALL(scm_boolean_eq, make_bool(false))));
	ck_assert(sexp_bool(CALL(scm_boolean_eq, make_bool(true), make_bool(true))));
	ck_assert(!sexp_bool(CALL(scm_boolean_eq, make_bool(true), make_bool(false))));
}
END_TEST

Suite *arithmetic_suite(void)
{
	Suite *s;
	TCase *tc;

	s = suite_create("Arithmetic");
	tc = tcase_create("Core");
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
	suite_add_tcase(s, tc);

	return s;
}