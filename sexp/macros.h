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

#ifndef _SEXP_MACROS_H
#define _SEXP_MACROS_H

#define DEFUN(fname, aname) \
	sexp_t fname (sexp_t aname, env_t  ____env)
#define DEFMACRO DEFUN
#define CALL(fname, args) fname(args, ____env)

#define sexp_list_for_each(cons, head) \
	for (cons = (sexp_t) (head); sexp_type(cons) == SEXP_PAIR; \
			cons = sexp_pair(cons)->cdr)

#define sexp_list_for_each_safe(cons, n, head) \
	for (cons = (sexp_t) (head), n = cdr(head); \
			sexp_type(cons) == SEXP_PAIR; \
			cons = n, \
			n = (sexp_type(n) == SEXP_PAIR) ? cdr(n) : n)

#define sexp_zipped_for_each(cons_a, cons_b, head_a, head_b) \
	for (cons_a = (sexp_t) (head_a), cons_b = (sexp_t) (head_b); \
			sexp_type(cons_a) == SEXP_PAIR && \
			sexp_type(cons_b) == SEXP_PAIR; \
			cons_a = sexp_pair(cons_a)->cdr, \
			cons_b = sexp_pair(cons_b)->cdr)

#define sexp_for_each_arg(arg, valist) \
	for (sexp_t arg = va_arg(valist, sexp_t); \
			sexp_type(arg) != SEXP_VOID; \
			arg = va_arg(valist, sexp_t))

#endif
