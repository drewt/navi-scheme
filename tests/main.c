/* Copyright 2014-2015 Drew Thoreson
 *
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/.
 */

#include "test.h"

Suite *arithmetic_suite(void);

navi_obj eval(const char *str)
{
	navi_obj port = navi_open_input_string(navi_cstr_to_string(str));
	navi_obj result = navi_eval(navi_read(navi_port(port), env), env);
	navi_close_input_port(navi_port(port), env);
	return result;
}

int main(void)
{
	int failed;
	Suite *s;
	SRunner *sr;

	/* initialize navi */
	navi_init();
	env = navi_interaction_environment();

	/* initialize test suite */
	s = suite_create("NAVI Core");
	suite_add_tcase(s, arithmetic_tests());
	suite_add_tcase(s, bytevector_tests());
	suite_add_tcase(s, char_tests());
	suite_add_tcase(s, lambda_tests());
	suite_add_tcase(s, list_tests());
	sr = srunner_create(s);

	/* run tests */
	srunner_run_all(sr, CK_NORMAL);
	failed = srunner_ntests_failed(sr);
	srunner_free(sr);
	return failed ? EXIT_FAILURE : EXIT_SUCCESS;
}
