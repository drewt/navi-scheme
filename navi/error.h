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

#ifndef _NAVI_ERROR_H
#define _NAVI_ERROR_H

#include "types.h"
#include "symbols.h"

#define error(env, msg, ...) _error(env, msg, ##__VA_ARGS__, make_void())
_Noreturn void _error(env_t env, const char *msg, ...);

#define read_error(env, msg, ...) error(env, msg, sym_read_error, \
		##__VA_ARGS__, make_void())

static inline bool has_type(sexp_t sexp, enum sexp_type type)
{
	return sexp_type(sexp) == type;
}

static inline sexp_t type_check(sexp_t sexp, enum sexp_type type, env_t env)
{
	if (!has_type(sexp, type))
		error(env, "type error",
				make_apair("expected", typesym(type)),
				make_apair("actual", typesym(sexp_type(sexp))));
	return sexp;
}

static inline long type_check_range(sexp_t n, long min, long max, env_t env)
{
	type_check(n, SEXP_NUM, env);
	if (sexp_num(n) < min || sexp_num(n) >= max)
		error(env, "argument not in allowed range",
				make_apair("min", make_num(min)),
				make_apair("max", make_num(max)),
				make_apair("actual", n));
	return sexp_num(n);
}

static inline sexp_t type_check_list(sexp_t list, env_t env)
{
	if (!is_proper_list(list))
		error(env, "type error: not a proper list");
	return list;
}

static inline sexp_t type_check_fun(sexp_t fun, int arity, env_t env)
{
	type_check(fun, SEXP_FUNCTION, env);
	int actual = sexp_fun(fun)->arity;
	if (actual != arity)
		error(env, "wrong arity",
				make_apair("expected", make_num(arity)),
				make_apair("actual", make_num(actual)));
	return fun;
}

static inline long fixnum_cast(sexp_t sexp, env_t env)
{
	return sexp_num(type_check(sexp, SEXP_NUM, env));
}

static inline unsigned long char_cast(sexp_t sexp, env_t env)
{
	return sexp_char(type_check(sexp, SEXP_CHAR, env));
}

static inline struct sexp_string *string_cast(sexp_t sexp, env_t env)
{
	return sexp_string(type_check(sexp, SEXP_STRING, env));
}

static inline struct sexp_bytevec *bytevec_cast(sexp_t sexp,
		enum sexp_type type, env_t env)
{
	return sexp_bytevec(type_check(sexp, type, env));
}

static inline struct sexp_vector *vector_cast(sexp_t sexp,
		enum sexp_type type, env_t env)
{
	return sexp_vector(type_check(sexp, type, env));
}

static inline struct sexp_port *port_cast(sexp_t sexp, env_t env)
{
	return sexp_port(type_check(sexp, SEXP_PORT, env));
}

static inline unsigned char type_check_byte(sexp_t sexp, env_t env)
{
	return type_check_range(sexp, 0, 256, env);
}

static inline void check_copy_to(size_t to, long at, size_t from, long start,
		long end, env_t env)
{
	if (at < 0 || (size_t)at >= to || start < 0 || (size_t)start >= from
			|| end < start || (size_t)end > from
			|| to - at < (size_t)(end - start))
		error(env, "invalid indices for copy");
}

#define check_copy(size, start, end, env) \
	check_copy_to(size, 0, size, start, end, env)

#endif
