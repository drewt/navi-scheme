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

#define DEFSPECIAL(fname, aname, ename) \
	sexp_t fname (sexp_t aname, env_t ename)
#define DEFUN(fname, aname) \
	DEFSPECIAL(fname, aname, ____env)
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

/* Stolen from chicken.h
;
; Copyright (c) 2008-2013, The Chicken Team
; Copyright (c) 2000-2007, Felix L. Winkelmann
; All rights reserved.
;
; Redistribution and use in source and binary forms, with or without modification, are permitted provided that the following
; conditions are met:
;
;   Redistributions of source code must retain the above copyright notice, this list of conditions and the following
;     disclaimer.
;   Redistributions in binary form must reproduce the above copyright notice, this list of conditions and the following
;     disclaimer in the documentation and/or other materials provided with the distribution.
;   Neither the name of the author nor the names of its contributors may be used to endorse or promote
;     products derived from this software without specific prior written permission.
;
; THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS" AND ANY EXPRESS
; OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY
; AND FITNESS FOR A PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT HOLDERS OR
; CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR
; CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR
; SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY
; THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR
; OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE
; POSSIBILITY OF SUCH DAMAGE.
*/


#define fixnum_fix(n)   (((n) << 1) | 1)
#define fixnum_unfix(n) ((n) >> 1)

#define u_fixnum_plus(n1, n2)  (((n1) - 1) + (n2))
#define fixnum_plus(n1, n2)    (u_fixnum_plus(n1, n2) | 1)
#define u_fixnum_minus(n1, n2) ((n1) - (n2) + 1)
#define fixnum_minus(n1, n2)   (u_fixnum_minus(n1, n2) | 1)
#define fixnum_times(n1, n2)   (fixnum_fix(fixnum_unfix(n1) * fixnum_unfix(n2)))
#define fixnum_divide(n1, n2)  (fixnum_fix(fixnum_unfix(n1) / fixnum_unfix(n2)))
#define fixnum_modulo(n1, n2)  (fixnum_fix(fixnum_unfix(n1) % fixnum_unfix(n2)))


#endif
