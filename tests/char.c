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

#include "test.h"

/* char? */
START_TEST(test_charp)
{
	ck_assert(sexp_bool($(scm_charp, make_char('a'))));
	ck_assert(!sexp_bool($(scm_charp, make_num(1))));
}
END_TEST

/* char<? */
START_TEST(test_char_lt)
{
	ck_assert(sexp_bool($(scm_char_lt, make_char('a'), make_char('b'))));
	ck_assert(!sexp_bool($(scm_char_lt, make_char('a'), make_char('a'))));
	ck_assert(!sexp_bool($(scm_char_lt, make_char('b'), make_char('a'))));
}
END_TEST

/* char>? */
START_TEST(test_char_gt)
{
	ck_assert(sexp_bool($(scm_char_gt, make_char('b'), make_char('a'))));
	ck_assert(!sexp_bool($(scm_char_gt, make_char('b'), make_char('b'))));
	ck_assert(!sexp_bool($(scm_char_gt, make_char('a'), make_char('b'))));
}
END_TEST

/* char=? */
START_TEST(test_char_eq)
{
	ck_assert(sexp_bool($(scm_char_eq, make_char('a'), make_char('a'))));
	ck_assert(!sexp_bool($(scm_char_eq, make_char('a'), make_char('b'))));
}
END_TEST

/* char<=? */
START_TEST(test_char_lte)
{
	ck_assert(sexp_bool($(scm_char_lte, make_char('a'), make_char('a'))));
	ck_assert(sexp_bool($(scm_char_lte, make_char('a'), make_char('b'))));
	ck_assert(!sexp_bool($(scm_char_lte, make_char('b'), make_char('a'))));
}
END_TEST

/* char>=? */
START_TEST(test_char_gte)
{
	ck_assert(sexp_bool($(scm_char_gte, make_char('a'), make_char('a'))));
	ck_assert(sexp_bool($(scm_char_gte, make_char('b'), make_char('a'))));
	ck_assert(!sexp_bool($(scm_char_gte, make_char('a'), make_char('b'))));
}
END_TEST

/* char-ci<? */
START_TEST(test_char_ci_lt)
{
	ck_assert(sexp_bool($(scm_char_ci_lt, make_char('a'), make_char('b'))));
	ck_assert(!sexp_bool($(scm_char_ci_lt, make_char('a'), make_char('a'))));
	ck_assert(!sexp_bool($(scm_char_ci_lt, make_char('b'), make_char('a'))));
	ck_assert(sexp_bool($(scm_char_ci_lt, make_char('a'), make_char('B'))));
	ck_assert(!sexp_bool($(scm_char_ci_lt, make_char('A'), make_char('a'))));
	ck_assert(!sexp_bool($(scm_char_ci_lt, make_char('b'), make_char('A'))));
}
END_TEST

/* char-ci>? */
START_TEST(test_char_ci_gt)
{
	ck_assert(sexp_bool($(scm_char_ci_gt, make_char('b'), make_char('a'))));
	ck_assert(!sexp_bool($(scm_char_ci_gt, make_char('b'), make_char('b'))));
	ck_assert(!sexp_bool($(scm_char_ci_gt, make_char('a'), make_char('b'))));
	ck_assert(sexp_bool($(scm_char_ci_gt, make_char('B'), make_char('a'))));
	ck_assert(!sexp_bool($(scm_char_ci_gt, make_char('b'), make_char('B'))));
	ck_assert(!sexp_bool($(scm_char_ci_gt, make_char('A'), make_char('b'))));
}
END_TEST

/* char-ci=? */
START_TEST(test_char_ci_eq)
{
	ck_assert(sexp_bool($(scm_char_ci_eq, make_char('a'), make_char('a'))));
	ck_assert(sexp_bool($(scm_char_ci_eq, make_char('A'), make_char('a'))));
	ck_assert(!sexp_bool($(scm_char_ci_eq, make_char('a'), make_char('B'))));
}
END_TEST

/* char-ci<=? */
START_TEST(test_char_ci_lte)
{
	ck_assert(sexp_bool($(scm_char_ci_lte, make_char('a'), make_char('a'))));
	ck_assert(sexp_bool($(scm_char_ci_lte, make_char('a'), make_char('b'))));
	ck_assert(!sexp_bool($(scm_char_ci_lte, make_char('b'), make_char('a'))));
	ck_assert(sexp_bool($(scm_char_ci_lte, make_char('A'), make_char('a'))));
	ck_assert(sexp_bool($(scm_char_ci_lte, make_char('a'), make_char('B'))));
	ck_assert(!sexp_bool($(scm_char_ci_lte, make_char('B'), make_char('a'))));
}
END_TEST

/* char-ci>=? */
START_TEST(test_char_ci_gte)
{
	ck_assert(sexp_bool($(scm_char_ci_gte, make_char('a'), make_char('a'))));
	ck_assert(sexp_bool($(scm_char_ci_gte, make_char('b'), make_char('a'))));
	ck_assert(!sexp_bool($(scm_char_ci_gte, make_char('a'), make_char('b'))));
	ck_assert(sexp_bool($(scm_char_ci_gte, make_char('a'), make_char('A'))));
	ck_assert(sexp_bool($(scm_char_ci_gte, make_char('B'), make_char('a'))));
	ck_assert(!sexp_bool($(scm_char_ci_gte, make_char('a'), make_char('B'))));

}
END_TEST

/* char-upcase */
START_TEST(test_char_upcase)
{
	ck_assert_int_eq(sexp_char($(scm_char_upcase, make_char('a'))), 'A');
	ck_assert_int_eq(sexp_char($(scm_char_upcase, make_char('A'))), 'A');
}
END_TEST

/* char-downcase */
START_TEST(test_char_downcase)
{
	ck_assert_int_eq(sexp_char($(scm_char_downcase, make_char('A'))), 'a');
	ck_assert_int_eq(sexp_char($(scm_char_downcase, make_char('a'))), 'a');
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
