/* Copyright 2014-2015 Drew Thoreson
 *
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/.
 */

#ifndef _TESTS_TEST_H
#define _TESTS_TEST_H

#include <stdlib.h>
#include <check.h>

#define $$(fn) fn(0, navi_make_nil(), env, NULL)
#define $(fn, ...) \
	fn(navi_list_length(navi_list(__VA_ARGS__, navi_make_void())), \
			navi_list(__VA_ARGS__, navi_make_void()), env, NULL)

navi_env env;

navi_obj eval(const char *str);
void assert_0_to_3(navi_obj list);

#define assert_num_eq(o, n) \
	do { \
		ck_assert(navi_is_fixnum(o)); \
		ck_assert_int_eq(navi_fixnum(o), n); \
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
