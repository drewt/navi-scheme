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

#include "test.h"

START_TEST(test_thunk)
{
	navi_obj o = eval("(lambda () 1)");
	ck_assert(navi_is_procedure(o));
	ck_assert(!navi_proc_is_variadic(navi_procedure(o)));
	ck_assert_uint_eq(navi_procedure(o)->arity, 0);
	ck_assert(navi_is_nil(navi_procedure(o)->args));

	o = eval("((lambda () 1))");
	assert_num_eq(o, 1);
}
END_TEST

START_TEST(test_single_argument)
{
	navi_obj o = eval("(lambda (x) x)");
	ck_assert(navi_is_procedure(o));
	ck_assert(!navi_proc_is_variadic(navi_procedure(o)));
	ck_assert_uint_eq(navi_procedure(o)->arity, 1);
	ck_assert(navi_is_pair(navi_procedure(o)->args));
	ck_assert_int_eq(navi_list_length(navi_procedure(o)->args), 1);

	o = eval("((lambda (x) x) 1)");
	assert_num_eq(o, 1);
}
END_TEST

START_TEST(test_multiple_arguments)
{
	navi_obj o = eval("(lambda (x y) y)");
	ck_assert(navi_is_procedure(o));
	ck_assert(!navi_proc_is_variadic(navi_procedure(o)));
	ck_assert_uint_eq(navi_procedure(o)->arity, 2);
	ck_assert(navi_is_pair(navi_procedure(o)->args));
	ck_assert_int_eq(navi_list_length(navi_procedure(o)->args), 2);

	o = eval("((lambda (x y) y) 2 1)");
	assert_num_eq(o, 1);
}
END_TEST

START_TEST(test_variadic)
{
	navi_obj o = eval("(lambda x x)");
	ck_assert(navi_is_procedure(o));
	ck_assert(navi_proc_is_variadic(navi_procedure(o)));
	ck_assert_uint_eq(navi_procedure(o)->arity, 0);
	ck_assert(navi_is_symbol(navi_procedure(o)->args));
	assert_0_to_3(eval("((lambda x x) 0 1 2 3)"));
}
END_TEST

START_TEST(test_variadic_with_fixed)
{
	navi_obj o = eval("(lambda (fst . rest) rest)");
	ck_assert(navi_is_procedure(o));
	ck_assert(navi_proc_is_variadic(navi_procedure(o)));
	ck_assert_uint_eq(navi_procedure(o)->arity, 1);
	assert_0_to_3(eval("((lambda (fst . rest) rest) 4 0 1 2 3)"));
	assert_0_to_3(eval("((lambda (fst . rest) fst) '(0 1 2 3))"));
}
END_TEST

START_TEST(test_apply)
{
	assert_num_eq(eval("(apply + '())"), 0);
	assert_num_eq(eval("(apply + '(1))"), 1);
	assert_num_eq(eval("(apply + '(1 1))"), 2);
	assert_num_eq(eval("(apply + 1 '())"), 1);
	assert_num_eq(eval("(apply + 1 '(1))"), 2);
	assert_num_eq(eval("(apply + 1 1 '(1))"), 3);
	// TODO: order of arguments
}
END_TEST

TCase *lambda_tests(void)
{
	TCase *tc = tcase_create("Lambda");
	tcase_add_test(tc, test_thunk);
	tcase_add_test(tc, test_single_argument);
	tcase_add_test(tc, test_multiple_arguments);
	tcase_add_test(tc, test_variadic);
	tcase_add_test(tc, test_variadic_with_fixed);
	tcase_add_test(tc, test_apply);
	return tc;
}
