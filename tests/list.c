/* Copyright 2014-2015 Drew Thoreson
 *
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/.
 */

#include "test.h"

/* pair? */
START_TEST(test_pairp)
{
	navi_obj o = eval("(pair? '(1 . 2))");
	ck_assert(navi_is_bool(o));
	ck_assert(navi_bool(o));
	ck_assert(!navi_bool(eval("(pair? 1)")));
}
END_TEST

/* cons */
START_TEST(test_cons)
{
	navi_obj o = eval("(cons 1 2)");
	ck_assert(navi_is_pair(o));
	assert_num_eq(navi_car(o), 1);
	assert_num_eq(navi_cdr(o), 2);
}
END_TEST

/* car */
START_TEST(test_car)
{
	assert_num_eq(eval("(car '(1 . 2))"), 1);
}
END_TEST

/* cdr */
START_TEST(test_cdr)
{
	assert_num_eq(eval("(cdr '(1 . 2))"), 2);
}
END_TEST

/* set-car! */
START_TEST(test_set_car)
{
	assert_num_eq(eval("(let ((p '(1 . 2))) (set-car! p 3) (car p))"), 3);
}
END_TEST

/* set-cdr! */
START_TEST(test_set_cdr)
{
	assert_num_eq(eval("(let ((p '(1 . 2))) (set-cdr! p 4) (cdr p))"), 4);
}
END_TEST

/* null? */
START_TEST(test_nullp)
{
	assert_bool_true(eval("(null? '())"));
	assert_bool_false(eval("(null? 1)"));
}
END_TEST

/* list? */
START_TEST(test_listp)
{
	assert_bool_true(eval("(list? '())"));
	assert_bool_true(eval("(list? '(1))"));
	assert_bool_false(eval("(list? '(1 . 2))"));
	assert_bool_false(eval("(list? 1)"));
}
END_TEST

/* make-list */
START_TEST(test_make_list)
{
	ck_assert(navi_is_nil(eval("(make-list 0)")));
	ck_assert(navi_is_pair(eval("(make-list 1)")));

	int i = 0;
	navi_obj cons, o = eval("(make-list 5 3)");
	navi_list_for_each(cons, o) {
		assert_num_eq(navi_car(cons), 3);
		i++;
	}
	ck_assert(navi_is_nil(cons));
	ck_assert_int_eq(i, 5);
}
END_TEST

void assert_0_to_3(navi_obj list)
{
	int i = 0;
	navi_obj cons;
	navi_list_for_each(cons, list) {
		assert_num_eq(navi_car(cons), i);
		i++;
	}
	ck_assert(navi_is_nil(cons));
	ck_assert_int_eq(i, 4);
}

/* list */
START_TEST(test_list)
{
	ck_assert(navi_is_nil(eval("(list)")));
	ck_assert(navi_is_pair(eval("(list 1)")));
	assert_0_to_3(eval("(list 0 1 2 3)"));
}
END_TEST

/* length */
START_TEST(test_length)
{
	assert_num_eq(eval("(length '())"), 0);
	assert_num_eq(eval("(length '(1))"), 1);
	assert_num_eq(eval("(length '(1 2))"), 2);
}
END_TEST

/* append */
START_TEST(test_append)
{
	ck_assert(navi_is_nil(eval("(append '() '())")));
	assert_num_eq(eval("(append '() 1)"), 1);
	assert_0_to_3(eval("(append '(0 1) '(2 3))"));
}
END_TEST

/* reverse */
START_TEST(test_reverse)
{
	ck_assert(navi_is_nil(eval("(reverse '())")));
	ck_assert(navi_is_pair(eval("(reverse '(1))")));
	assert_0_to_3(eval("(reverse '(3 2 1 0))"));
}
END_TEST

/* list-tail */
START_TEST(test_list_tail)
{
	ck_assert(navi_is_nil(eval("(list-tail '() 0)")));
	ck_assert(navi_is_pair(eval("(list-tail '(1) 0)")));
	assert_0_to_3(eval("(list-tail '(4 5 6 0 1 2 3) 3)"));
}
END_TEST

/* list-ref */
START_TEST(test_list_ref)
{
	assert_num_eq(eval("(list-ref '(2 1 0) 0)"), 2);
	assert_num_eq(eval("(list-ref '(2 1 0) 1)"), 1);
	assert_num_eq(eval("(list-ref '(2 1 0) 2)"), 0);
}
END_TEST

/* list-set! */
START_TEST(test_list_set)
{
	assert_0_to_3(eval("(let ((x '(4 1 2 3))) (list-set! x 0 0) x)"));
	assert_0_to_3(eval("(let ((x '(0 4 2 3))) (list-set! x 1 1) x)"));
	assert_0_to_3(eval("(let ((x '(0 1 4 3))) (list-set! x 2 2) x)"));
	assert_0_to_3(eval("(let ((x '(0 1 2 4))) (list-set! x 3 3) x)"));
}
END_TEST

/* memq */
START_TEST(test_memq)
{
	assert_bool_false(eval("(memq 1 '())"));
	ck_assert(navi_is_pair(eval("(memq 1 '(1))")));
	assert_0_to_3(eval("(memq 0 '(3 2 1 0 1 2 3))"));
}
END_TEST

/* memv */
START_TEST(test_memv)
{
	assert_bool_false(eval("(memv 1 '())"));
	ck_assert(navi_is_pair(eval("(memv 1 '(1))")));
	assert_0_to_3(eval("(memv 0 '(3 2 1 0 1 2 3))"));
}
END_TEST

/* member */
START_TEST(test_member)
{
	assert_bool_false(eval("(member \"1\" '())"));
	ck_assert(navi_is_pair(eval("(member \"1\" '(\"1\"))")));
	assert_0_to_3(eval("(member 0 '(3 2 1 0 1 2 3))"));
	assert_0_to_3(eval("(member -1 '(-1 -2 -3 0 1 2 3) <)"));
}
END_TEST

/* assq */
START_TEST(test_assq)
{
	assert_bool_false(eval("(assq 1 '())"));
	ck_assert(navi_is_pair(eval("(assq 1 '((1 2)))")));
	assert_0_to_3(eval("(assq 0 '((3) (2) (1) (0 1 2 3)))"));
}
END_TEST

/* assv */
START_TEST(test_assv)
{
	assert_bool_false(eval("(assv 1 '())"));
	ck_assert(navi_is_pair(eval("(assv 1 '((1 2)))")));
	assert_0_to_3(eval("(assv 0 '((3) (2) (1) (0 1 2 3)))"));
}
END_TEST

/* assoc */
START_TEST(test_assoc)
{
	assert_bool_false(eval("(assoc \"1\" '())"));
	ck_assert(navi_is_pair(eval("(assoc \"1\" '((\"1\")))")));
	assert_0_to_3(eval("(assoc 0 '((3) (2) (1) (0 1 2 3)))"));
	assert_0_to_3(eval("(assoc -1 '((-1) (-2) (-3) (0 1 2 3)) <)"));
}
END_TEST

/* list-copy */
START_TEST(test_list_copy)
{
	ck_assert(navi_is_nil(eval("(list-copy '())")));
	ck_assert(navi_is_pair(eval("(list-copy '(1))")));
	assert_bool_false(eval("(let ((x '(1))) (eq? x (list-copy x)))"));
	assert_0_to_3(eval("(list-copy '(0 1 2 3))"));
}
END_TEST

/* map */
START_TEST(test_map)
{
	ck_assert(navi_is_nil(eval("(map + '())")));
	assert_0_to_3(eval("(map (lambda (x) (- x 1)) '(1 2 3 4))"));
	assert_0_to_3(eval("(map - '(4 4 4 4) '(4 3 2 1))"));
	assert_0_to_3(eval("(map - '(4 4 4 4 4) '(4 3 2 1))"));
}
END_TEST

/* for-each */
START_TEST(test_for_each)
{
	// XXX: this tests for in-order evaluation
	assert_0_to_3(eval("(let ((x '()))"
				"(for-each (lambda (y) (set! x (cons y x)))"
					"'(3 2 1 0)) x)"));
	assert_0_to_3(eval("(let ((x '()))"
				"(for-each (lambda (y z) (set! x (cons (+ y z) x)))"
					"'(2 1 0 -1) '(1 1 1 1)) x)"));
	assert_0_to_3(eval("(let ((x '()))"
				"(for-each (lambda (y z) (set! x (cons (+ y z) x)))"
					"'(2 1 0 -1) '(1 1 1 1 1)) x)"));
}
END_TEST

TCase *list_tests(void)
{
	TCase *tc = tcase_create("Pairs and Lists");
	tcase_add_test(tc, test_append);
	tcase_add_test(tc, test_assoc);
	tcase_add_test(tc, test_assq);
	tcase_add_test(tc, test_assv);
	tcase_add_test(tc, test_car);
	tcase_add_test(tc, test_cdr);
	tcase_add_test(tc, test_cons);
	tcase_add_test(tc, test_for_each);
	tcase_add_test(tc, test_length);
	tcase_add_test(tc, test_list);
	tcase_add_test(tc, test_list_copy);
	tcase_add_test(tc, test_list_ref);
	tcase_add_test(tc, test_list_set);
	tcase_add_test(tc, test_list_tail);
	tcase_add_test(tc, test_listp);
	tcase_add_test(tc, test_make_list);
	tcase_add_test(tc, test_map);
	tcase_add_test(tc, test_member);
	tcase_add_test(tc, test_memq);
	tcase_add_test(tc, test_memv);
	tcase_add_test(tc, test_nullp);
	tcase_add_test(tc, test_pairp);
	tcase_add_test(tc, test_reverse);
	tcase_add_test(tc, test_set_car);
	tcase_add_test(tc, test_set_cdr);
	return tc;
}
