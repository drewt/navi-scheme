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

#define bytevector_assert(v, sz) \
	ck_assert(sexp_type(v) == SEXP_BYTEVEC && sexp_bytevec(v)->size == sz)

#define bytevector_assert_ref(v, i, n) \
	ck_assert_int_eq(sexp_bytevec(v)->data[i], n)

/* bytevector? */
START_TEST(test_bytevectorp)
{
	ck_assert(sexp_bool($(scm_bytevectorp, sexp_make_bytevec(1))));
	ck_assert(!sexp_bool($(scm_bytevectorp, sexp_make_num(1))));
}
END_TEST

/* make-bytevector */
START_TEST(test_make_bytevector)
{
	sexp_t v = $(scm_make_bytevector, sexp_make_num(2));
	bytevector_assert(v, 2);

	v = $(scm_make_bytevector, sexp_make_num(2), sexp_make_num(3));
	bytevector_assert(v, 2);
	bytevector_assert_ref(v, 0, 3);
	bytevector_assert_ref(v, 1, 3);
}
END_TEST

/* bytevector */
START_TEST(test_bytevector)
{
	sexp_t v = $$(scm_bytevector);
	bytevector_assert(v, 0);

	v = $(scm_bytevector, sexp_make_num(1), sexp_make_num(2));
	bytevector_assert(v, 2);
	bytevector_assert_ref(v, 0, 1);
	bytevector_assert_ref(v, 1, 2);
}
END_TEST

/* bytevector-length */
START_TEST(test_bytevector_length)
{
	sexp_t v = sexp_make_bytevec(2);
	ck_assert(sexp_num($(scm_bytevector_length, v)) == 2);
}
END_TEST

/* bytevector-u8-ref */
START_TEST(test_bytevector_u8_ref)
{
	sexp_t v = sexp_make_bytevec(2);
	sexp_bytevec(v)->data[0] = 1;
	sexp_bytevec(v)->data[1] = 2;
	ck_assert(sexp_num($(scm_bytevector_u8_ref, v, sexp_make_num(0))) == 1);
	ck_assert(sexp_num($(scm_bytevector_u8_ref, v, sexp_make_num(1))) == 2);
}
END_TEST

/* bytevector-u8-set! */
START_TEST(test_bytevector_u8_set)
{
	sexp_t v = sexp_make_bytevec(2);
	sexp_bytevec(v)->data[0] = 0;
	sexp_bytevec(v)->data[1] = 1;
	$(scm_bytevector_u8_set, v, sexp_make_num(0), sexp_make_num(1));
	$(scm_bytevector_u8_set, v, sexp_make_num(1), sexp_make_num(2));
	bytevector_assert_ref(v, 0, 1);
	bytevector_assert_ref(v, 1, 2);
}
END_TEST

/* bytevector-append */
START_TEST(test_bytevector_append)
{
	sexp_t v0 = $(scm_bytevector, sexp_make_num(1), sexp_make_num(2));
	sexp_t v1 = $(scm_bytevector, sexp_make_num(3), sexp_make_num(4));
	sexp_t v3 = $(scm_bytevector_append, v0, v1);
	bytevector_assert(v3, 4);
	bytevector_assert_ref(v3, 0, 1);
	bytevector_assert_ref(v3, 1, 2);
	bytevector_assert_ref(v3, 2, 3);
	bytevector_assert_ref(v3, 3, 4);
}
END_TEST

/* bytevector-copy */
START_TEST(test_bytevector_copy)
{
	sexp_t v;
	sexp_t o = $(scm_bytevector, sexp_make_num(0), sexp_make_num(1), sexp_make_num(2), sexp_make_num(3));

	v = $(scm_bytevector_copy, o);
	bytevector_assert(v, 4);
	bytevector_assert_ref(v, 0, 0);
	bytevector_assert_ref(v, 1, 1);
	bytevector_assert_ref(v, 2, 2);
	bytevector_assert_ref(v, 3, 3);

	v = $(scm_bytevector_copy, o, sexp_make_num(2));
	bytevector_assert(v, 2);
	bytevector_assert_ref(v, 0, 2);
	bytevector_assert_ref(v, 1, 3);

	v = $(scm_bytevector_copy, o, sexp_make_num(1), sexp_make_num(3));
	bytevector_assert(v, 2);
	bytevector_assert_ref(v, 0, 1);
	bytevector_assert_ref(v, 1, 2);
}
END_TEST

/* bytevector-copy! */
START_TEST(test_bytevector_copy_to)
{
	sexp_t v = $(scm_make_bytevector, sexp_make_num(4), sexp_make_num(4));
	sexp_t o = $(scm_bytevector, sexp_make_num(0), sexp_make_num(1), sexp_make_num(2), sexp_make_num(3));

	$(scm_bytevector_copy_to, v, sexp_make_num(0), o);
	bytevector_assert_ref(v, 0, 0);
	bytevector_assert_ref(v, 1, 1);
	bytevector_assert_ref(v, 2, 2);
	bytevector_assert_ref(v, 3, 3);

	sexp_bytevec(v)->data[3] = 4;
	$(scm_bytevector_copy_to, v, sexp_make_num(3), o, sexp_make_num(3));
	bytevector_assert_ref(v, 3, 3);

	sexp_bytevec(v)->data[1] = 4;
	$(scm_bytevector_copy_to, v, sexp_make_num(1), o, sexp_make_num(2), sexp_make_num(3));
	bytevector_assert_ref(v, 1, 2);

}
END_TEST

/* utf8->string */
START_TEST(test_utf8_to_string)
{
	sexp_t v = $(scm_bytevector, sexp_make_num(0xCE), sexp_make_num(0xBB));
	sexp_t s = $(scm_utf8_to_string, v);
	ck_assert_uint_eq(sexp_type(s), SEXP_STRING);
	ck_assert_uint_eq((unsigned char)sexp_string(s)->data[0], 0xCE);
	ck_assert_uint_eq((unsigned char)sexp_string(s)->data[1], 0xBB);
	ck_assert_uint_eq((unsigned char)sexp_string(s)->data[2], '\0');
}
END_TEST

/* string->utf8 */
START_TEST(test_string_to_utf8)
{
	sexp_t s = $(scm_string, sexp_make_char(0x3BB));
	sexp_t v = $(scm_string_to_utf8, s);
	bytevector_assert(v, 2);
	bytevector_assert_ref(v, 0, 0xCE);
	bytevector_assert_ref(v, 1, 0xBB);
}
END_TEST

TCase *bytevector_tests(void)
{
	TCase *tc = tcase_create("Bytevectors");
	tcase_add_test(tc, test_bytevectorp);
	tcase_add_test(tc, test_make_bytevector);
	tcase_add_test(tc, test_bytevector);
	tcase_add_test(tc, test_bytevector_length);
	tcase_add_test(tc, test_bytevector_u8_ref);
	tcase_add_test(tc, test_bytevector_u8_set);
	tcase_add_test(tc, test_bytevector_append);
	tcase_add_test(tc, test_bytevector_copy);
	tcase_add_test(tc, test_bytevector_copy_to);
	tcase_add_test(tc, test_utf8_to_string);
	tcase_add_test(tc, test_string_to_utf8);
	return tc;
}
