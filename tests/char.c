/* Copyright 2014-2015 Drew Thoreson
 *
 * This file is part of libnavi.
 *
 * libnavi is free software: you can redistribute it and/or modify it under
 * the terms of the GNU Lesser General Public License as published by the Free
 * Software Foundation, version 3 of the License.
 *
 * libnavi is distributed in the hope that it will be useful, but WITHOUT ANY
 * WARRANTY; without even the implied warranty of MERCHANTABILITY or FITNESS
 * FOR A PARTICULAR PURPOSE.  See the GNU Lesser General Public License for
 * more details.
 *
 * You should have received a copy of the GNU Lesser General Public License
 * along with libnavi.  If not, see <http://www.gnu.org/licenses/>.
 */

#include "test.h"

/* char? */
START_TEST(test_charp)
{
	ck_assert(navi_bool($(scm_charp, navi_make_char('a'))));
	ck_assert(!navi_bool($(scm_charp, navi_make_fixnum(1))));
}
END_TEST

/* char<? */
START_TEST(test_char_lt)
{
	ck_assert(navi_bool($(scm_char_lt, navi_make_char('a'), navi_make_char('b'))));
	ck_assert(!navi_bool($(scm_char_lt, navi_make_char('a'), navi_make_char('a'))));
	ck_assert(!navi_bool($(scm_char_lt, navi_make_char('b'), navi_make_char('a'))));
}
END_TEST

/* char>? */
START_TEST(test_char_gt)
{
	ck_assert(navi_bool($(scm_char_gt, navi_make_char('b'), navi_make_char('a'))));
	ck_assert(!navi_bool($(scm_char_gt, navi_make_char('b'), navi_make_char('b'))));
	ck_assert(!navi_bool($(scm_char_gt, navi_make_char('a'), navi_make_char('b'))));
}
END_TEST

/* char=? */
START_TEST(test_char_eq)
{
	ck_assert(navi_bool($(scm_char_eq, navi_make_char('a'), navi_make_char('a'))));
	ck_assert(!navi_bool($(scm_char_eq, navi_make_char('a'), navi_make_char('b'))));
}
END_TEST

/* char<=? */
START_TEST(test_char_lte)
{
	ck_assert(navi_bool($(scm_char_lte, navi_make_char('a'), navi_make_char('a'))));
	ck_assert(navi_bool($(scm_char_lte, navi_make_char('a'), navi_make_char('b'))));
	ck_assert(!navi_bool($(scm_char_lte, navi_make_char('b'), navi_make_char('a'))));
}
END_TEST

/* char>=? */
START_TEST(test_char_gte)
{
	ck_assert(navi_bool($(scm_char_gte, navi_make_char('a'), navi_make_char('a'))));
	ck_assert(navi_bool($(scm_char_gte, navi_make_char('b'), navi_make_char('a'))));
	ck_assert(!navi_bool($(scm_char_gte, navi_make_char('a'), navi_make_char('b'))));
}
END_TEST

/* char-ci<? */
START_TEST(test_char_ci_lt)
{
	ck_assert(navi_bool($(scm_char_ci_lt, navi_make_char('a'), navi_make_char('b'))));
	ck_assert(!navi_bool($(scm_char_ci_lt, navi_make_char('a'), navi_make_char('a'))));
	ck_assert(!navi_bool($(scm_char_ci_lt, navi_make_char('b'), navi_make_char('a'))));
	ck_assert(navi_bool($(scm_char_ci_lt, navi_make_char('a'), navi_make_char('B'))));
	ck_assert(!navi_bool($(scm_char_ci_lt, navi_make_char('A'), navi_make_char('a'))));
	ck_assert(!navi_bool($(scm_char_ci_lt, navi_make_char('b'), navi_make_char('A'))));
}
END_TEST

/* char-ci>? */
START_TEST(test_char_ci_gt)
{
	ck_assert(navi_bool($(scm_char_ci_gt, navi_make_char('b'), navi_make_char('a'))));
	ck_assert(!navi_bool($(scm_char_ci_gt, navi_make_char('b'), navi_make_char('b'))));
	ck_assert(!navi_bool($(scm_char_ci_gt, navi_make_char('a'), navi_make_char('b'))));
	ck_assert(navi_bool($(scm_char_ci_gt, navi_make_char('B'), navi_make_char('a'))));
	ck_assert(!navi_bool($(scm_char_ci_gt, navi_make_char('b'), navi_make_char('B'))));
	ck_assert(!navi_bool($(scm_char_ci_gt, navi_make_char('A'), navi_make_char('b'))));
}
END_TEST

/* char-ci=? */
START_TEST(test_char_ci_eq)
{
	ck_assert(navi_bool($(scm_char_ci_eq, navi_make_char('a'), navi_make_char('a'))));
	ck_assert(navi_bool($(scm_char_ci_eq, navi_make_char('A'), navi_make_char('a'))));
	ck_assert(!navi_bool($(scm_char_ci_eq, navi_make_char('a'), navi_make_char('B'))));
}
END_TEST

/* char-ci<=? */
START_TEST(test_char_ci_lte)
{
	ck_assert(navi_bool($(scm_char_ci_lte, navi_make_char('a'), navi_make_char('a'))));
	ck_assert(navi_bool($(scm_char_ci_lte, navi_make_char('a'), navi_make_char('b'))));
	ck_assert(!navi_bool($(scm_char_ci_lte, navi_make_char('b'), navi_make_char('a'))));
	ck_assert(navi_bool($(scm_char_ci_lte, navi_make_char('A'), navi_make_char('a'))));
	ck_assert(navi_bool($(scm_char_ci_lte, navi_make_char('a'), navi_make_char('B'))));
	ck_assert(!navi_bool($(scm_char_ci_lte, navi_make_char('B'), navi_make_char('a'))));
}
END_TEST

/* char-ci>=? */
START_TEST(test_char_ci_gte)
{
	ck_assert(navi_bool($(scm_char_ci_gte, navi_make_char('a'), navi_make_char('a'))));
	ck_assert(navi_bool($(scm_char_ci_gte, navi_make_char('b'), navi_make_char('a'))));
	ck_assert(!navi_bool($(scm_char_ci_gte, navi_make_char('a'), navi_make_char('b'))));
	ck_assert(navi_bool($(scm_char_ci_gte, navi_make_char('a'), navi_make_char('A'))));
	ck_assert(navi_bool($(scm_char_ci_gte, navi_make_char('B'), navi_make_char('a'))));
	ck_assert(!navi_bool($(scm_char_ci_gte, navi_make_char('a'), navi_make_char('B'))));

}
END_TEST

/* char-upcase */
START_TEST(test_char_upcase)
{
	ck_assert_int_eq(navi_char($(scm_char_upcase, navi_make_char('a'))), 'A');
	ck_assert_int_eq(navi_char($(scm_char_upcase, navi_make_char('A'))), 'A');
}
END_TEST

/* char-downcase */
START_TEST(test_char_downcase)
{
	ck_assert_int_eq(navi_char($(scm_char_downcase, navi_make_char('A'))), 'a');
	ck_assert_int_eq(navi_char($(scm_char_downcase, navi_make_char('a'))), 'a');
}
END_TEST

TCase *char_tests(void)
{
	TCase *tc = tcase_create("Characters");
	tcase_add_test(tc, test_charp);
	tcase_add_test(tc, test_char_lt);
	tcase_add_test(tc, test_char_gt);
	tcase_add_test(tc, test_char_eq);
	tcase_add_test(tc, test_char_lte);
	tcase_add_test(tc, test_char_gte);
	tcase_add_test(tc, test_char_ci_lt);
	tcase_add_test(tc, test_char_ci_gt);
	tcase_add_test(tc, test_char_ci_eq);
	tcase_add_test(tc, test_char_ci_lte);
	tcase_add_test(tc, test_char_ci_gte);
	tcase_add_test(tc, test_char_upcase);
	tcase_add_test(tc, test_char_downcase);
	return tc;
}
