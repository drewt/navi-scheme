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
	ck_assert(sexp_bool($(scm_charp, sexp_make_char('a'))));
	ck_assert(!sexp_bool($(scm_charp, sexp_make_num(1))));
}
END_TEST

/* char<? */
START_TEST(test_char_lt)
{
	ck_assert(sexp_bool($(scm_char_lt, sexp_make_char('a'), sexp_make_char('b'))));
	ck_assert(!sexp_bool($(scm_char_lt, sexp_make_char('a'), sexp_make_char('a'))));
	ck_assert(!sexp_bool($(scm_char_lt, sexp_make_char('b'), sexp_make_char('a'))));
}
END_TEST

/* char>? */
START_TEST(test_char_gt)
{
	ck_assert(sexp_bool($(scm_char_gt, sexp_make_char('b'), sexp_make_char('a'))));
	ck_assert(!sexp_bool($(scm_char_gt, sexp_make_char('b'), sexp_make_char('b'))));
	ck_assert(!sexp_bool($(scm_char_gt, sexp_make_char('a'), sexp_make_char('b'))));
}
END_TEST

/* char=? */
START_TEST(test_char_eq)
{
	ck_assert(sexp_bool($(scm_char_eq, sexp_make_char('a'), sexp_make_char('a'))));
	ck_assert(!sexp_bool($(scm_char_eq, sexp_make_char('a'), sexp_make_char('b'))));
}
END_TEST

/* char<=? */
START_TEST(test_char_lte)
{
	ck_assert(sexp_bool($(scm_char_lte, sexp_make_char('a'), sexp_make_char('a'))));
	ck_assert(sexp_bool($(scm_char_lte, sexp_make_char('a'), sexp_make_char('b'))));
	ck_assert(!sexp_bool($(scm_char_lte, sexp_make_char('b'), sexp_make_char('a'))));
}
END_TEST

/* char>=? */
START_TEST(test_char_gte)
{
	ck_assert(sexp_bool($(scm_char_gte, sexp_make_char('a'), sexp_make_char('a'))));
	ck_assert(sexp_bool($(scm_char_gte, sexp_make_char('b'), sexp_make_char('a'))));
	ck_assert(!sexp_bool($(scm_char_gte, sexp_make_char('a'), sexp_make_char('b'))));
}
END_TEST

/* char-ci<? */
START_TEST(test_char_ci_lt)
{
	ck_assert(sexp_bool($(scm_char_ci_lt, sexp_make_char('a'), sexp_make_char('b'))));
	ck_assert(!sexp_bool($(scm_char_ci_lt, sexp_make_char('a'), sexp_make_char('a'))));
	ck_assert(!sexp_bool($(scm_char_ci_lt, sexp_make_char('b'), sexp_make_char('a'))));
	ck_assert(sexp_bool($(scm_char_ci_lt, sexp_make_char('a'), sexp_make_char('B'))));
	ck_assert(!sexp_bool($(scm_char_ci_lt, sexp_make_char('A'), sexp_make_char('a'))));
	ck_assert(!sexp_bool($(scm_char_ci_lt, sexp_make_char('b'), sexp_make_char('A'))));
}
END_TEST

/* char-ci>? */
START_TEST(test_char_ci_gt)
{
	ck_assert(sexp_bool($(scm_char_ci_gt, sexp_make_char('b'), sexp_make_char('a'))));
	ck_assert(!sexp_bool($(scm_char_ci_gt, sexp_make_char('b'), sexp_make_char('b'))));
	ck_assert(!sexp_bool($(scm_char_ci_gt, sexp_make_char('a'), sexp_make_char('b'))));
	ck_assert(sexp_bool($(scm_char_ci_gt, sexp_make_char('B'), sexp_make_char('a'))));
	ck_assert(!sexp_bool($(scm_char_ci_gt, sexp_make_char('b'), sexp_make_char('B'))));
	ck_assert(!sexp_bool($(scm_char_ci_gt, sexp_make_char('A'), sexp_make_char('b'))));
}
END_TEST

/* char-ci=? */
START_TEST(test_char_ci_eq)
{
	ck_assert(sexp_bool($(scm_char_ci_eq, sexp_make_char('a'), sexp_make_char('a'))));
	ck_assert(sexp_bool($(scm_char_ci_eq, sexp_make_char('A'), sexp_make_char('a'))));
	ck_assert(!sexp_bool($(scm_char_ci_eq, sexp_make_char('a'), sexp_make_char('B'))));
}
END_TEST

/* char-ci<=? */
START_TEST(test_char_ci_lte)
{
	ck_assert(sexp_bool($(scm_char_ci_lte, sexp_make_char('a'), sexp_make_char('a'))));
	ck_assert(sexp_bool($(scm_char_ci_lte, sexp_make_char('a'), sexp_make_char('b'))));
	ck_assert(!sexp_bool($(scm_char_ci_lte, sexp_make_char('b'), sexp_make_char('a'))));
	ck_assert(sexp_bool($(scm_char_ci_lte, sexp_make_char('A'), sexp_make_char('a'))));
	ck_assert(sexp_bool($(scm_char_ci_lte, sexp_make_char('a'), sexp_make_char('B'))));
	ck_assert(!sexp_bool($(scm_char_ci_lte, sexp_make_char('B'), sexp_make_char('a'))));
}
END_TEST

/* char-ci>=? */
START_TEST(test_char_ci_gte)
{
	ck_assert(sexp_bool($(scm_char_ci_gte, sexp_make_char('a'), sexp_make_char('a'))));
	ck_assert(sexp_bool($(scm_char_ci_gte, sexp_make_char('b'), sexp_make_char('a'))));
	ck_assert(!sexp_bool($(scm_char_ci_gte, sexp_make_char('a'), sexp_make_char('b'))));
	ck_assert(sexp_bool($(scm_char_ci_gte, sexp_make_char('a'), sexp_make_char('A'))));
	ck_assert(sexp_bool($(scm_char_ci_gte, sexp_make_char('B'), sexp_make_char('a'))));
	ck_assert(!sexp_bool($(scm_char_ci_gte, sexp_make_char('a'), sexp_make_char('B'))));

}
END_TEST

/* char-upcase */
START_TEST(test_char_upcase)
{
	ck_assert_int_eq(sexp_char($(scm_char_upcase, sexp_make_char('a'))), 'A');
	ck_assert_int_eq(sexp_char($(scm_char_upcase, sexp_make_char('A'))), 'A');
}
END_TEST

/* char-downcase */
START_TEST(test_char_downcase)
{
	ck_assert_int_eq(sexp_char($(scm_char_downcase, sexp_make_char('A'))), 'a');
	ck_assert_int_eq(sexp_char($(scm_char_downcase, sexp_make_char('a'))), 'a');
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
