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

#ifndef _TESTS_TEST_H
#define _TESTS_TEST_H

#include <stdlib.h>
#include <check.h>
#include "../navi.h"

#define $$(fn) fn(0, navi_make_nil(), env)
#define $(fn, ...) \
	fn(navi_list_length(navi_list(__VA_ARGS__, navi_make_void())), \
			navi_list(__VA_ARGS__, navi_make_void()), env)

navi_env env;

navi_obj eval(const char *str);
void assert_0_to_3(navi_obj list);

#define assert_num_eq(o, n) \
	do { \
		ck_assert(navi_is_num(o)); \
		ck_assert_int_eq(navi_num(o), n); \
	} while (0)

#define assert_bool_true(b) \
	do { \
		ck_assert(navi_is_bool(b)); \
		ck_assert(navi_bool(b)); \
	} while (0)

#define assert_bool_false(b) \
	do { \
		ck_assert(navi_is_bool(b)); \
		ck_assert(!navi_bool(b)); \
	} while (0)

TCase *arithmetic_tests(void);
TCase *char_tests(void);
TCase *bytevector_tests(void);
TCase *lambda_tests(void);
TCase *list_tests(void);

#endif
