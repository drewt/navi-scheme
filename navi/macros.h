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

#ifndef _NAVI_MACROS_H
#define _NAVI_MACROS_H

#include "types.h"

#define DEFPROC(_type, cname, args, env, name, _arity, _flags, ...) \
	int scm_typedecl_##cname[_arity] = { __VA_ARGS__ }; \
	navi_obj scm_##cname(navi_obj args, navi_env env); \
	struct navi_spec scm_decl_##cname = { \
		.proc = { \
			.flags = _flags | NAVI_PROC_BUILTIN, \
			.arity = _arity, \
			.c_proc = scm_##cname, \
		}, \
		.ident = name, \
		.type  = _type, \
	}; \
	navi_obj scm_##cname(navi_obj args, navi_env env)

#define DEFUN(...)      DEFPROC(NAVI_PROCEDURE, __VA_ARGS__)
#define DEFMACRO(...)   DEFPROC(NAVI_MACRO,     __VA_ARGS__)
#define DEFSPECIAL(...) DEFPROC(NAVI_SPECIAL,   __VA_ARGS__)

/*
 * XXX: really, the functions should be declared static and not exposed here,
 *      but it's useful to call them directly for unit testing.
 */
#define DECLARE(name) \
	extern struct navi_spec scm_decl_##name; \
	navi_obj scm_##name(navi_obj, navi_env)

#define navi_list_for_each(cons, head) \
	for (cons = (navi_obj) (head); navi_type(cons) == NAVI_PAIR; \
			cons = navi_pair(cons)->cdr)

#define navi_list_for_each_safe(cons, n, head) \
	for (cons = (navi_obj) (head), n = navi_cdr(head); \
			navi_type(cons) == NAVI_PAIR; \
			cons = n, \
			n = (navi_type(n) == NAVI_PAIR) ? navi_cdr(n) : n)

#define navi_list_for_each_zipped(cons_a, cons_b, head_a, head_b) \
	for (cons_a = (navi_obj) (head_a), cons_b = (navi_obj) (head_b); \
			navi_type(cons_a) == NAVI_PAIR && \
			navi_type(cons_b) == NAVI_PAIR; \
			cons_a = navi_pair(cons_a)->cdr, \
			cons_b = navi_pair(cons_b)->cdr)

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


#define navi_fixnum_fix(n)   (((n) << 1) | 1)
#define navi_fixnum_unfix(n) ((n) >> 1)

#define navi_u_fixnum_plus(n1, n2)  (((n1) - 1) + (n2))
#define navi_fixnum_plus(n1, n2)    (navi_u_fixnum_plus(n1, n2) | 1)
#define navi_u_fixnum_minus(n1, n2) ((n1) - (n2) + 1)
#define navi_fixnum_minus(n1, n2)   (navi_u_fixnum_minus(n1, n2) | 1)
#define navi_fixnum_times(n1, n2)   (navi_fixnum_fix(navi_fixnum_unfix(n1) * navi_fixnum_unfix(n2)))
#define navi_fixnum_divide(n1, n2)  (navi_fixnum_fix(navi_fixnum_unfix(n1) / navi_fixnum_unfix(n2)))
#define navi_fixnum_modulo(n1, n2)  (navi_fixnum_fix(navi_fixnum_unfix(n1) % navi_fixnum_unfix(n2)))


#endif
