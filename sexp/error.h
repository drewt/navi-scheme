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

#ifndef _SEXP_ERROR_H
#define _SEXP_ERROR_H

#include "types.h"
#include "symbols.h"

#define error(env, msg, ...) _error(env, msg, ##__VA_ARGS__, make_void())
_Noreturn void _error(env_t env, const char *msg, ...);

/*
 * Type Checking
 *
 * Functions prefixed with an underscore take an explicit env_t argument, and
 * are meant for use outside of DEFUNs.  Without the underscore, the magic
 * variable ____env (which is only available in a DEFUN) is used.
 */

static inline bool has_type(sexp_t sexp, enum sexp_type type)
{
	return sexp_type(sexp) == type;
}

#define type_check(sexp, type) _type_check(sexp, type, ____env)
static inline sexp_t _type_check(sexp_t sexp, enum sexp_type type, env_t env)
{
	if (!has_type(sexp, type))
		error(env, "type error",
				make_apair("expected", typesym(type)),
				make_apair("actual", typesym(sexp_type(sexp))));
	return sexp;
}

#define type_check_range(n, min, max) _type_check_range(n, min, max, ____env)
static inline sexp_t _type_check_range(sexp_t n, long min, long max, env_t env)
{
	_type_check(n, SEXP_NUM, env);
	if (sexp_num(n) < min || sexp_num(n) >= max)
		error(env, "argument not in allowed range",
				make_apair("min", make_num(min)),
				make_apair("max", make_num(max)),
				make_apair("actual", n));
	return n;
}

#define type_check_list(sexp) _type_check_list(sexp, ____env)
static inline sexp_t _type_check_list(sexp_t list, env_t env)
{
	if (!is_proper_list(list))
		error(env, "type error: not a proper list");
	return list;
}

#define type_check_fun(sexp, arity) _type_check_fun(sexp, arity, ____env)
static inline sexp_t _type_check_fun(sexp_t fun, int arity, env_t env)
{
	_type_check(fun, SEXP_FUNCTION, env);
	int actual = sexp_fun(fun)->arity;
	if (actual != arity)
		error(env, "wrong arity",
				make_apair("expected", make_num(arity)),
				make_apair("actual", make_num(actual)));
	return fun;
}

#define fixnum_cast(sexp) _fixnum_cast(sexp, ____env)
static inline long _fixnum_cast(sexp_t sexp, env_t env)
{
	return sexp_num(_type_check(sexp, SEXP_NUM, env));
}

#define char_cast(sexp) _char_cast(sexp, ____env)
static inline unsigned long _char_cast(sexp_t sexp, env_t env)
{
	return sexp_char(_type_check(sexp, SEXP_CHAR, env));
}

#define bytevec_cast(sexp, type) _bytevec_cast(sexp, type, ____env)
static inline struct sexp_bytevec *_bytevec_cast(sexp_t sexp,
		enum sexp_type type, env_t env)
{
	return sexp_bytevec(_type_check(sexp, type, env));
}

#define vector_cast(sexp, type) _vector_cast(sexp, type, ____env)
static inline struct sexp_vector *_vector_cast(sexp_t sexp,
		enum sexp_type type, env_t env)
{
	return sexp_vector(_type_check(sexp, type, env));
}

#define type_check_byte(sexp) _type_check_byte(sexp, ____env)
static inline unsigned char _type_check_byte(sexp_t sexp, env_t env)
{
	return sexp_num(_type_check_range(sexp, 0, 256, env));
}

#endif
