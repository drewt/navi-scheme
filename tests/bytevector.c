/* Copyright 2014-2015 Drew Thoreson
 *
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/.
 */

#include "test.h"

#define bytevector_assert(v, sz) \
	ck_assert(navi_type(v) == NAVI_BYTEVEC && navi_bytevec(v)->size == sz)

#define bytevector_assert_ref(v, i, n) \
	ck_assert_int_eq(navi_bytevec(v)->data[i], n)

/* bytevector? */
START_TEST(test_bytevectorp)
{
	ck_assert(navi_bool($(scm_bytevectorp, navi_make_bytevec(1))));
	ck_assert(!navi_bool($(scm_bytevectorp, navi_make_fixnum(1))));
}
END_TEST

/* make-bytevector */
START_TEST(test_make_bytevector)
{
	navi_obj v = $(scm_make_bytevector, navi_make_fixnum(2));
	bytevector_assert(v, 2);

	v = $(scm_make_bytevector, navi_make_fixnum(2), navi_make_fixnum(3));
	bytevector_assert(v, 2);
	bytevector_assert_ref(v, 0, 3);
	bytevector_assert_ref(v, 1, 3);
}
END_TEST

/* bytevector */
START_TEST(test_bytevector)
{
	navi_obj v = $$(scm_bytevector);
	bytevector_assert(v, 0);

	v = $(scm_bytevector, navi_make_fixnum(1), navi_make_fixnum(2));
	bytevector_assert(v, 2);
	bytevector_assert_ref(v, 0, 1);
	bytevector_assert_ref(v, 1, 2);
}
END_TEST

/* bytevector-length */
START_TEST(test_bytevector_length)
{
	navi_obj v = navi_make_bytevec(2);
	ck_assert(navi_fixnum($(scm_bytevector_length, v)) == 2);
}
END_TEST

/* bytevector-u8-ref */
START_TEST(test_bytevector_u8_ref)
{
	navi_obj v = navi_make_bytevec(2);
	navi_bytevec(v)->data[0] = 1;
	navi_bytevec(v)->data[1] = 2;
	ck_assert(navi_fixnum($(scm_bytevector_u8_ref, v, navi_make_fixnum(0))) == 1);
	ck_assert(navi_fixnum($(scm_bytevector_u8_ref, v, navi_make_fixnum(1))) == 2);
}
END_TEST

/* bytevector-u8-set! */
START_TEST(test_bytevector_u8_set)
{
	navi_obj v = navi_make_bytevec(2);
	navi_bytevec(v)->data[0] = 0;
	navi_bytevec(v)->data[1] = 1;
	$(scm_bytevector_u8_set, v, navi_make_fixnum(0), navi_make_fixnum(1));
	$(scm_bytevector_u8_set, v, navi_make_fixnum(1), navi_make_fixnum(2));
	bytevector_assert_ref(v, 0, 1);
	bytevector_assert_ref(v, 1, 2);
}
END_TEST

/* bytevector-append */
START_TEST(test_bytevector_append)
{
	navi_obj v0 = $(scm_bytevector, navi_make_fixnum(1), navi_make_fixnum(2));
	navi_obj v1 = $(scm_bytevector, navi_make_fixnum(3), navi_make_fixnum(4));
	navi_obj v3 = $(scm_bytevector_append, v0, v1);
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
	navi_obj v;
	navi_obj o = $(scm_bytevector, navi_make_fixnum(0), navi_make_fixnum(1), navi_make_fixnum(2), navi_make_fixnum(3));

	v = $(scm_bytevector_copy, o);
	bytevector_assert(v, 4);
	bytevector_assert_ref(v, 0, 0);
	bytevector_assert_ref(v, 1, 1);
	bytevector_assert_ref(v, 2, 2);
	bytevector_assert_ref(v, 3, 3);

	v = $(scm_bytevector_copy, o, navi_make_fixnum(2));
	bytevector_assert(v, 2);
	bytevector_assert_ref(v, 0, 2);
	bytevector_assert_ref(v, 1, 3);

	v = $(scm_bytevector_copy, o, navi_make_fixnum(1), navi_make_fixnum(3));
	bytevector_assert(v, 2);
	bytevector_assert_ref(v, 0, 1);
	bytevector_assert_ref(v, 1, 2);
}
END_TEST

/* bytevector-copy! */
START_TEST(test_bytevector_copy_to)
{
	navi_obj v = $(scm_make_bytevector, navi_make_fixnum(4), navi_make_fixnum(4));
	navi_obj o = $(scm_bytevector, navi_make_fixnum(0), navi_make_fixnum(1), navi_make_fixnum(2), navi_make_fixnum(3));

	$(scm_bytevector_copy_to, v, navi_make_fixnum(0), o);
	bytevector_assert_ref(v, 0, 0);
	bytevector_assert_ref(v, 1, 1);
	bytevector_assert_ref(v, 2, 2);
	bytevector_assert_ref(v, 3, 3);

	navi_bytevec(v)->data[3] = 4;
	$(scm_bytevector_copy_to, v, navi_make_fixnum(3), o, navi_make_fixnum(3));
	bytevector_assert_ref(v, 3, 3);

	navi_bytevec(v)->data[1] = 4;
	$(scm_bytevector_copy_to, v, navi_make_fixnum(1), o, navi_make_fixnum(2), navi_make_fixnum(3));
	bytevector_assert_ref(v, 1, 2);

}
END_TEST

/* utf8->string */
START_TEST(test_utf8_to_string)
{
	navi_obj v = $(scm_bytevector, navi_make_fixnum(0xCE), navi_make_fixnum(0xBB));
	navi_obj s = $(scm_utf8_to_string, v);
	ck_assert_uint_eq(navi_type(s), NAVI_STRING);
	ck_assert_uint_eq((unsigned char)navi_string(s)->data[0], 0xCE);
	ck_assert_uint_eq((unsigned char)navi_string(s)->data[1], 0xBB);
	ck_assert_uint_eq((unsigned char)navi_string(s)->data[2], '\0');
}
END_TEST

/* string->utf8 */
START_TEST(test_string_to_utf8)
{
	navi_obj s = $(scm_string, navi_make_char('a'), navi_make_char('b'));
	navi_obj v = $(scm_string_to_utf8, s);
	bytevector_assert(v, 2);
	bytevector_assert_ref(v, 0, 'a');
	bytevector_assert_ref(v, 1, 'b');
#ifdef HAVE_ICU
	s = $(scm_string, navi_make_char(0x3BB));
	v = $(scm_string_to_utf8, s);
	bytevector_assert(v, 2);
	bytevector_assert_ref(v, 0, 0xCE);
	bytevector_assert_ref(v, 1, 0xBB);
#endif
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
