/* Copyright 2014-2015 Drew Thoreson
 *
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/.
 */

#ifndef _NAVI_ERROR_H
#define _NAVI_ERROR_H

#include <stdio.h>
#include "internal.h"
#include "symbols.h"

_Noreturn void _navi_error(navi_env env, navi_obj type, const char *msg, ...);
#define navi_error(env, ...) \
	_navi_error(env, navi_sym_internal_error, __VA_ARGS__, navi_make_void())
#define navi_read_error(env, ...) \
	_navi_error(env, navi_sym_read_error, __VA_ARGS__, navi_make_void())
#define navi_file_error(env, ...) \
	_navi_error(env, navi_sym_file_error, __VA_ARGS__, navi_make_void())
#define navi_enomem(env) \
	navi_error(env, "not enough memory")
#define navi_arity_error(env, name) \
	navi_error(env, "wrong number of arguments", navi_make_apair("target", name))
#define navi_unbound_identifier_error(env, ident) \
	navi_error(env, "unbound identifier", navi_make_apair("identifier", ident))
#define navi_type_error(env, expected, actual) \
	navi_error(env, "type error", \
			navi_make_apair("expected", expected), \
			navi_make_apair("actual", actual))

static inline navi_obj navi_type_check(navi_obj obj, enum navi_type type, navi_env env)
{
	if (unlikely(navi_type(obj) != type))
		navi_type_error(env, navi_typesym(type), navi_typesym(navi_type(obj)));
	return obj;
}

static inline long navi_type_check_range(navi_obj n, long min, long max, navi_env env)
{
	navi_type_check(n, NAVI_FIXNUM, env);
	if (unlikely(navi_fixnum(n) < min || navi_fixnum(n) >= max))
		navi_error(env, "argument not in allowed range",
				navi_make_apair("min", navi_make_fixnum(min)),
				navi_make_apair("max", navi_make_fixnum(max)),
				navi_make_apair("actual", n));
	return navi_fixnum(n);
}

static inline navi_obj navi_type_check_list(navi_obj list, navi_env env)
{
	if (unlikely(!navi_is_list(list)))
		navi_type_error(env, navi_make_symbol("list"),
				navi_typesym(navi_type(list)));
	return list;
}

static inline navi_obj navi_type_check_proper_list(navi_obj list, navi_env env)
{
	if (unlikely(!navi_is_proper_list(list)))
		navi_error(env, "type error: not a proper list");
	return list;
}

static inline navi_obj navi_check_arity(navi_obj proc, int arity, navi_env env)
{
	int actual = navi_procedure(proc)->arity;
	if (unlikely(!navi_arity_satisfied(navi_procedure(proc), arity)))
		navi_error(env, "wrong arity",
				navi_make_apair("expected", navi_make_fixnum(arity)),
				navi_make_apair("actual", navi_make_fixnum(actual)));
	return proc;
}

static inline navi_obj navi_type_check_proc(navi_obj proc, int arity, navi_env env)
{
	navi_type_check(proc, NAVI_PROCEDURE, env);
	return navi_check_arity(proc, arity, env);
}

static inline long navi_fixnum_cast(navi_obj num, navi_env env)
{
	return navi_fixnum(navi_type_check(num, NAVI_FIXNUM, env));
}

static inline unsigned long navi_char_cast(navi_obj ch, navi_env env)
{
	return navi_char(navi_type_check(ch, NAVI_CHAR, env));
}

static inline struct navi_string *navi_string_cast(navi_obj str, navi_env env)
{
	return navi_string(navi_type_check(str, NAVI_STRING, env));
}

static inline struct navi_bytevec *navi_bytevec_cast(navi_obj vec, navi_env env)
{
	return navi_bytevec(navi_type_check(vec, NAVI_BYTEVEC, env));
}

static inline struct navi_vector *navi_vector_cast(navi_obj vec, navi_env env)
{
	return navi_vector(navi_type_check(vec, NAVI_VECTOR, env));
}

static inline struct navi_port *navi_port_cast(navi_obj port, navi_env env)
{
	return navi_port(navi_type_check(port, NAVI_PORT, env));
}

static inline unsigned char navi_type_check_byte(navi_obj byte, navi_env env)
{
	return navi_type_check_range(byte, 0, 256, env);
}

static inline void navi_check_copy_to(size_t to, long at, size_t from,
		long start, long end, navi_env env)
{
	if (unlikely(at < 0 || (size_t)at >= to || start < 0
			|| (size_t)start >= from
			|| end < start || (size_t)end > from
			|| to - at < (size_t)(end - start)))
		navi_error(env, "invalid indices for copy");
}

#define navi_check_copy(size, start, end, env) \
	navi_check_copy_to(size, 0, size, start, end, env)

#endif
