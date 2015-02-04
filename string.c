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

#include <string.h>
#include <ctype.h>

#include "navi.h"
#include "navi/uchar.h"

static navi_t list_to_string(navi_t list, navi_env_t env)
{
	navi_t cons;
	size_t size = 0, length = 0;

	navi_list_for_each(cons, list) {
		navi_type_check(navi_car(cons), NAVI_CHAR, env);
		size += u_char_size(navi_char(navi_car(cons)));
		length++;
	}

	size_t i = 0;
	navi_t expr = navi_make_string(size, size, length);
	struct navi_string *str = navi_string(expr);

	navi_list_for_each(cons, list) {
		u_set_char((char*)str->data, &i, navi_char(navi_car(cons)));
	}
	return expr;
}

static navi_t string_to_list(navi_t expr)
{
	struct navi_pair head, *ptr;
	struct navi_string *string = navi_string (expr);

	ptr = &head;
	for (size_t i = 0; i < string->size;) {
		ptr->cdr = navi_make_empty_pair();
		ptr = navi_pair(ptr->cdr);
		ptr->car = navi_make_char(u_get_char(string->data, &i));
	}
	ptr->cdr = navi_make_nil();
	return head.cdr;
}

char *navi_string_to_cstr(navi_t expr)
{
	struct navi_string *string = navi_string(expr);
	char *cstr = malloc(string->size + 1);
	if (!cstr)
		return NULL;
	for (size_t i = 0; i < string->size; i++)
		cstr[i] = string->data[i];
	cstr[string->size] = '\0';
	return cstr;
}

DEFUN(scm_stringp, args, env)
{
	return navi_make_bool(navi_type(navi_car(args)) == NAVI_STRING);
}

DEFUN(scm_make_string, args, env)
{
	uchar ch = ' ';
	int nr_args = navi_list_length(args);

	long length = navi_fixnum_cast(navi_car(args), env);

	if (length < 0)
		navi_error(env, "invalid length");
	if (nr_args > 1)
		ch = navi_char_cast(navi_cadr(args), env);

	/* FIXME: invalid codepoint? */
	size_t size = length * u_char_size(ch);
	navi_t expr = navi_make_string(size, size, length);

	size_t j = 0;
	struct navi_string *string = navi_string(expr);
	for (size_t i = 0; i < (size_t) length; i++)
		u_set_char_raw((char*)string->data, &j, ch);

	return expr;
}

DEFUN(scm_string, args, env)
{
	return list_to_string(args, env);
}

DEFUN(scm_string_length, args, env)
{
	return navi_make_num(navi_string_cast(navi_car(args), env)->length);
}

DEFUN(scm_string_ref, args, env)
{
	size_t i = 0;
	struct navi_string *str = navi_string_cast(navi_car(args), env);
	long k = navi_type_check_range(navi_cadr(args), 0, str->length, env);

	u_skip_chars(str->data, k, &i);
	return navi_make_char(u_get_char(str->data, &i));
}

DEFUN(scm_string_set, args, env)
{
	long k;
	struct navi_string *str;

	navi_type_check(navi_car(args),   NAVI_STRING, env);
	navi_type_check(navi_cadr(args),  NAVI_NUM,    env);
	navi_type_check(navi_caddr(args), NAVI_CHAR,   env);

	str = navi_string(navi_car(args));
	k = navi_num(navi_cadr(args));

	if (k < 0 || (size_t) k >= str->size)
		navi_die("string index out of bounds");

	// FIXME: UTF-8
	str->data[k] = navi_char(navi_caddr(args));
	return navi_unspecified();
}

#define BINARY_PREDICATE(cname, op) \
	DEFUN(cname, args, env) \
	{ \
		struct navi_string *fst, *snd; \
		\
		navi_type_check(navi_car(args),  NAVI_STRING, env); \
		navi_type_check(navi_cadr(args), NAVI_STRING, env); \
		\
		fst = navi_string(navi_car(args)); \
		snd = navi_string(navi_cadr(args)); \
		\
		if (fst->size != snd->size) \
			return navi_make_bool(false); \
		\
		for (size_t i = 0; i < fst->size; i++) { \
			if (!(fst->data[i] op snd->data[i])) \
				return navi_make_bool(false); \
		} \
		return navi_make_bool(true); \
	}

#define BINARY_CI_PREDICATE(cname, op) \
	DEFUN(cname, args, env) \
	{ \
		struct navi_string *fst, *snd; \
		\
		navi_type_check(navi_car(args),  NAVI_STRING, env); \
		navi_type_check(navi_cadr(args), NAVI_STRING, env); \
		\
		fst = navi_string(navi_car(args)); \
		snd = navi_string(navi_cadr(args)); \
		\
		if (fst->size != snd->size) \
			return navi_make_bool(false); \
		\
		for (size_t i = 0; i < fst->size; i++) { \
			if (!(tolower(fst->data[i]) op tolower(snd->data[i]))) \
				return navi_make_bool(false); \
		} \
		return navi_make_bool(true); \
	}

BINARY_PREDICATE(scm_string_lt,  <)
BINARY_PREDICATE(scm_string_gt,  >)
BINARY_PREDICATE(scm_string_eq,  ==)
BINARY_PREDICATE(scm_string_lte, <=)
BINARY_PREDICATE(scm_string_gte, >=)

BINARY_CI_PREDICATE(scm_string_ci_lt,  <)
BINARY_CI_PREDICATE(scm_string_ci_gt,  >)
BINARY_CI_PREDICATE(scm_string_ci_eq,  ==)
BINARY_CI_PREDICATE(scm_string_ci_lte, <=)
BINARY_CI_PREDICATE(scm_string_ci_gte, >=)

DEFUN(scm_substring, args, env)
{
	return scm_string_copy(args, env);
}

DEFUN(scm_string_append, args, env)
{
	navi_t cons, expr;
	struct navi_string *str;
	size_t i = 0, size = 0, length = 0;

	/* count combined size/length */
	navi_list_for_each(cons, args) {
		struct navi_string *s = navi_string_cast(navi_car(cons), env);
		size += s->size;
		length += s->length;
	}

	/* allocate */
	expr = navi_make_string(size, size, length);
	str = navi_string(expr);

	/* copy */
	navi_list_for_each(cons, args) {
		struct navi_string *other = navi_string(navi_car(cons));
		for (size_t j = 0; j < other->size; j++)
			str->data[i++] = other->data[j];
	}
	return expr;
}

DEFUN(scm_string_to_list, args, env)
{
	return string_to_list(navi_type_check(navi_car(args), NAVI_STRING, env));
}

DEFUN(scm_list_to_string, args, env)
{
	return list_to_string(navi_type_check_list(navi_car(args), env), env);
}

static navi_t copy_to(navi_t to, size_t at, navi_t from, size_t start,
		size_t end)
{
	struct navi_string *tos = navi_string(to), *froms = navi_string(from);
	for (size_t i = start; i < end; i++)
		tos->data[at++] = froms->data[i];
	return to;
}

DEFUN(scm_string_fill, args, env)
{
	navi_t ch;
	long end, start;
	struct navi_string *str;
	int nr_args = navi_list_length(args);

	str = navi_string_cast(navi_car(args), env);
	ch = navi_type_check(navi_cadr(args), NAVI_CHAR, env);
	start = (nr_args > 2) ? navi_fixnum_cast(navi_caddr(args), env) : 0;
	end = (nr_args > 3) ? navi_fixnum_cast(navi_cadddr(args), env) : (long) str->size;

	navi_check_copy(str->size, start, end, env);

	for (size_t i = start; i < (size_t) end; i++)
		str->data[i] = navi_char(ch);

	return navi_unspecified();
}

navi_t navi_string_copy(navi_t string)
{
	struct navi_string *from_str = navi_string(string);
	navi_t to = navi_make_string(from_str->size, from_str->size, from_str->size);
	struct navi_string *to_str = navi_string(to);

	for (size_t i = 0; i < from_str->size; i++)
		to_str->data[i] = from_str->data[i];

	return to;
}

DEFUN(scm_string_copy, args, env)
{
	navi_t from;
	long start, end;
	struct navi_string *str;
	int nr_args = navi_list_length(args);

	from = navi_type_check(navi_car(args), NAVI_STRING, env);
	str = navi_string(from);
	start = (nr_args > 1) ? navi_fixnum_cast(navi_cadr(args), env) : 0;
	end = (nr_args > 2) ? navi_fixnum_cast(navi_caddr(args), env) : (long) str->size;

	navi_check_copy(str->size, start, end, env);

	return copy_to(navi_make_string(end - start, end - start, end - start), 0,
			from, start, end);
}

DEFUN(scm_string_copy_to, args, env)
{
	navi_t to, from;
	long at, start, end;
	struct navi_string *from_str, *to_str;
	int nr_args = navi_list_length(args);

	to = navi_type_check(navi_car(args), NAVI_STRING, env);
	at = navi_fixnum_cast(navi_cadr(args), env);
	from = navi_type_check(navi_caddr(args), NAVI_STRING, env);
	to_str = navi_string(to);
	from_str = navi_string(from);
	start = (nr_args > 3) ? navi_fixnum_cast(navi_cadddr(args), env) : 0;
	end = (nr_args > 4) ? navi_fixnum_cast(navi_caddddr(args), env)
		: (long) from_str->size;

	navi_check_copy_to(to_str->size, at, from_str->size, start, end, env);

	copy_to(to, at, from, start, end);
	return navi_unspecified();
}

static navi_t string_map_ip(navi_t fun, navi_t str, navi_env_t env)
{
	struct navi_string *vec = navi_string_cast(str, env);

	for (size_t i = 0; i < vec->size; i++) {
		navi_t call = navi_list(fun, navi_make_char(vec->data[i]), navi_make_void());
		vec->data[i] = navi_char_cast(navi_eval(call, env), env);
	}
	return str;
}

DEFUN(scm_string_map_ip, args, env)
{
	navi_type_check_fun(navi_car(args), 1, env);
	navi_type_check(navi_cadr(args), NAVI_STRING, env);

	return string_map_ip(navi_car(args), navi_cadr(args), env);
}

DEFUN(scm_string_map, args, env)
{
	navi_type_check_fun(navi_car(args), 1, env);
	navi_type_check(navi_cadr(args), NAVI_STRING, env);

	return string_map_ip(navi_car(args), navi_string_copy(navi_cadr(args)), env);
}
