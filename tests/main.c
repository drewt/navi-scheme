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

Suite *arithmetic_suite(void);

int main(void)
{
	int failed;
	Suite *s;
	SRunner *sr;

	/* initialize navi */
	symbol_table_init();
	env = make_default_environment();

	/* initialize test suite */
	s = suite_create("NAVI Core");
	suite_add_tcase(s, arithmetic_tests());
	suite_add_tcase(s, bytevector_tests());
	suite_add_tcase(s, char_tests());
	sr = srunner_create(s);

	/* run tests */
	srunner_run_all(sr, CK_NORMAL);
	failed = srunner_ntests_failed(sr);
	srunner_free(sr);
	return failed ? EXIT_FAILURE : EXIT_SUCCESS;
}
