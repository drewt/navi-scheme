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
