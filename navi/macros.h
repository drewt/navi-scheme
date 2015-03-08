/* Copyright 2014-2015 Drew Thoreson
 *
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/.
 */

#ifndef _NAVI_MACROS_H
#define _NAVI_MACROS_H

#include "types.h"

#define SCM_DECL(name) scm_decl_##name

#define DEFPROC(_type, cname, scmname, _arity, _flags, ...)                  \
	static const int scm_typedecl_##cname[] = { __VA_ARGS__ };           \
	_Static_assert(sizeof(scm_typedecl_##cname)/sizeof(int) == _arity+1, \
			"DEFPROC: too few types for given arity");           \
	navi_obj scm_##cname(unsigned, navi_obj, navi_env,                   \
			struct navi_procedure*);                             \
	const struct navi_spec SCM_DECL(cname) = {                           \
		.proc = {                                                    \
			.flags  = _flags | NAVI_PROC_BUILTIN,                \
			.arity  = _arity,                                    \
			.c_proc = scm_##cname,                               \
			.types  = scm_typedecl_##cname,                      \
		},                                                           \
		.ident = scmname,                                            \
		.type  = _type,                                              \
	};                                                                   \
	navi_obj scm_##cname(unsigned scm_nr_args, navi_obj scm_args,        \
			navi_env scm_env, struct navi_procedure *scm_proc)

#define DEFUN(...)      DEFPROC(NAVI_PROCEDURE, __VA_ARGS__, 0)
#define DEFMACRO(...)   DEFPROC(NAVI_MACRO,     __VA_ARGS__, 0)
#define DEFSPECIAL(...) DEFPROC(NAVI_SPECIAL,   __VA_ARGS__, 0)

#define scm_arg1 navi_car(scm_args)
#define scm_arg2 navi_cadr(scm_args)
#define scm_arg3 navi_caddr(scm_args)
#define scm_arg4 navi_cadddr(scm_args)
#define scm_arg5 navi_caddddr(scm_args)

#define DEFPARAM(cname, scmname, value, converter)                           \
	const struct navi_spec SCM_DECL(cname) = {                           \
		.type = NAVI_PARAMETER,                                      \
		.param_value = &SCM_DECL(value),                             \
		.param_converter = &SCM_DECL(converter),                     \
		.ident = scmname,                                            \
	}

/*
 * XXX: really, the functions should be declared static and not exposed here,
 *      but it's useful to call them directly for unit testing.
 */
#define DECLARE(name)                                                        \
	extern const struct navi_spec SCM_DECL(name);                        \
	navi_obj scm_##name(unsigned, navi_obj, navi_env,                    \
			struct navi_procedure*)

/* Stolen from chicken.h
;
; Copyright (c) 2008-2013, The Chicken Team
; Copyright (c) 2000-2007, Felix L. Winkelmann
; All rights reserved.
;
; Redistribution and use in source and binary forms, with or without
; modification, are permitted provided that the following conditions are met:
;
;   Redistributions of source code must retain the above copyright notice,
;     this list of conditions and the following disclaimer.
;   Redistributions in binary form must reproduce the above copyright notice,
;     this list of conditions and the following disclaimer in the
;     documentation and/or other materials provided with the distribution.
;   Neither the name of the author nor the names of its contributors may be
;     used to endorse or promote products derived from this software without
;     specific prior written permission.
;
; THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS"
; AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
; IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
; ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT HOLDERS OR CONTRIBUTORS BE
; LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR
; CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF
; SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS
; INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN
; CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE)
; ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE
; POSSIBILITY OF SUCH DAMAGE.
*/

#define navi_fixnum_fix(n)   (((n) << 1) | 1)
#define navi_fixnum_unfix(n) ((n) >> 1)

#define navi_u_fixnum_plus(n1, n2)  (((n1) - 1) + (n2))
#define navi_fixnum_plus(n1, n2)    (navi_u_fixnum_plus(n1, n2) | 1)
#define navi_u_fixnum_minus(n1, n2) ((n1) - (n2) + 1)
#define navi_fixnum_minus(n1, n2)   (navi_u_fixnum_minus(n1, n2) | 1)
#define navi_fixnum_times(n1, n2)   (navi_fixnum_fix(navi_fixnum_unfix(n1) \
			* navi_fixnum_unfix(n2)))
#define navi_fixnum_divide(n1, n2)  (navi_fixnum_fix(navi_fixnum_unfix(n1) \
			/ navi_fixnum_unfix(n2)))
#define navi_fixnum_modulo(n1, n2)  (navi_fixnum_fix(navi_fixnum_unfix(n1) \
			% navi_fixnum_unfix(n2)))

#endif
