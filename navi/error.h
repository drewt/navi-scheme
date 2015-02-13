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

#ifndef _NAVI_ERROR_H
#define _NAVI_ERROR_H

#include <stdio.h>
#include "types.h"
#include "symbols.h"

_Noreturn void _navi_error(navi_env env, const char *msg, ...);
#define navi_error(env, msg, ...) \
	_navi_error(env, msg, ##__VA_ARGS__, navi_make_void())
#define navi_read_error(env, msg, ...) \
	navi_error(env, msg, navi_sym_read_error, ##__VA_ARGS__, navi_make_void())
#define navi_file_error(env, msg, ...) \
	navi_error(env, msg, navi_sym_file_error, ##__VA_ARGS__, navi_make_void())
#define navi_enomem(env) \
	navi_error(env, "not enough memory")
#define navi_arity_error(env, name) \
	navi_error(env, "wrong number of arguments", navi_make_apair("target", name))

static inline navi_obj navi_type_check(navi_obj obj, enum navi_type type, navi_env env)
{
	if (navi_type(obj) != type)
		navi_error(env, "type error",
				navi_make_apair("expected", navi_typesym(type)),
				navi_make_apair("actual", navi_typesym(navi_type(obj))));
	return obj;
}

static inline long navi_type_check_range(navi_obj n, long min, long max, navi_env env)
{
	navi_type_check(n, NAVI_NUM, env);
	if (navi_num(n) < min || navi_num(n) >= max)
		navi_error(env, "argument not in allowed range",
				navi_make_apair("min", navi_make_num(min)),
				navi_make_apair("max", navi_make_num(max)),
				navi_make_apair("actual", n));
	return navi_num(n);
}

static inline navi_obj navi_type_check_list(navi_obj list, navi_env env)
{
	if (!navi_is_proper_list(list))
		navi_error(env, "type error: not a proper list");
	return list;
}

static inline navi_obj navi_check_arity(navi_obj proc, int arity, navi_env env)
{
	int actual = navi_procedure(proc)->arity;
	if (actual != arity)
		navi_error(env, "wrong arity",
				navi_make_apair("expected", navi_make_num(arity)),
				navi_make_apair("actual", navi_make_num(actual)));
	return proc;
}

static inline navi_obj navi_type_check_proc(navi_obj proc, int arity, navi_env env)
{
	navi_type_check(proc, NAVI_PROCEDURE, env);
	return navi_check_arity(proc, arity, env);
}

static inline long navi_fixnum_cast(navi_obj num, navi_env env)
{
	return navi_num(navi_type_check(num, NAVI_NUM, env));
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

static inline struct navi_vector *navi_vector_cast(navi_obj vec,
		enum navi_type type, navi_env env)
{
	return navi_vector(navi_type_check(vec, type, env));
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
	if (at < 0 || (size_t)at >= to || start < 0 || (size_t)start >= from
			|| end < start || (size_t)end > from
			|| to - at < (size_t)(end - start))
		navi_error(env, "invalid indices for copy");
}

#define navi_check_copy(size, start, end, env) \
	navi_check_copy_to(size, 0, size, start, end, env)

#endif
