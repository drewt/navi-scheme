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

#include <string.h>
#include <ctype.h>

#include "sexp.h"

static sexp_t list_to_string(sexp_t list, env_t env)
{
	sexp_t cons;
	size_t size = 0, length = 0;

	sexp_list_for_each(cons, list) {
		type_check(car(cons), SEXP_CHAR, env);
		size += u_char_size(sexp_char(car(cons)));
		length++;
	}

	size_t i = 0;
	sexp_t sexp = make_string(size, size, length);
	struct sexp_string *str = sexp_string(sexp);

	sexp_list_for_each(cons, list) {
		u_set_char((char*)str->data, &i, sexp_char(car(cons)));
	}
	return sexp;
}

static sexp_t string_to_list(sexp_t sexp)
{
	struct sexp_pair head, *ptr;
	struct sexp_string *string = sexp_string (sexp);

	ptr = &head;
	for (size_t i = 0; i < string->size;) {
		ptr->cdr = make_empty_pair();
		ptr = sexp_pair(ptr->cdr);
		ptr->car = make_char(u_get_char(string->data, &i));
	}
	ptr->cdr = make_nil();
	return head.cdr;
}

char *scm_to_c_string(sexp_t sexp)
{
	struct sexp_string *string = sexp_string(sexp);
	char *cstr = xmalloc(string->size + 1);

	for (size_t i = 0; i < string->size; i++)
		cstr[i] = string->data[i];
	cstr[string->size] = '\0';
	return cstr;
}

DEFUN(scm_stringp, args, env)
{
	return make_bool(sexp_type(car(args)) == SEXP_STRING);
}

DEFUN(scm_make_string, args, env)
{
	uchar ch = ' ';
	int nr_args = list_length(args);

	long length = fixnum_cast(car(args), env);

	if (length < 0)
		error(env, "invalid length");
	if (nr_args > 1)
		ch = char_cast(cadr(args), env);

	/* FIXME: invalid codepoint? */
	size_t size = length * u_char_size(ch);
	sexp_t sexp = make_string(size, size, length);

	size_t j = 0;
	struct sexp_string *string = sexp_string(sexp);
	for (size_t i = 0; i < (size_t) length; i++)
		u_set_char_raw((char*)string->data, &j, ch);

	return sexp;
}

DEFUN(scm_string, args, env)
{
	return list_to_string(args, env);
}

DEFUN(scm_string_length, args, env)
{
	return make_num(string_cast(car(args), env)->length);
}

DEFUN(scm_string_ref, args, env)
{
	size_t i = 0;
	struct sexp_string *str = string_cast(car(args), env);
	long k = type_check_range(cadr(args), 0, str->length, env);

	u_skip_chars(str->data, k, &i);
	return make_char(u_get_char(str->data, &i));
}

DEFUN(scm_string_set, args, env)
{
	long k;
	struct sexp_string *str;

	type_check(car(args),   SEXP_STRING, env);
	type_check(cadr(args),  SEXP_NUM,    env);
	type_check(caddr(args), SEXP_CHAR,   env);

	str = sexp_string(car(args));
	k = sexp_num(cadr(args));

	if (k < 0 || (size_t) k >= str->size)
		die("string index out of bounds");

	// FIXME: UTF-8
	str->data[k] = sexp_char(caddr(args));
	return unspecified();
}

#define BINARY_PREDICATE(cname, op) \
	DEFUN(cname, args, env) \
	{ \
		struct sexp_string *fst, *snd; \
		\
		type_check(car(args),  SEXP_STRING, env); \
		type_check(cadr(args), SEXP_STRING, env); \
		\
		fst = sexp_string(car(args)); \
		snd = sexp_string(cadr(args)); \
		\
		if (fst->size != snd->size) \
			return make_bool(false); \
		\
		for (size_t i = 0; i < fst->size; i++) { \
			if (!(fst->data[i] op snd->data[i])) \
				return make_bool(false); \
		} \
		return make_bool(true); \
	}

#define BINARY_CI_PREDICATE(cname, op) \
	DEFUN(cname, args, env) \
	{ \
		struct sexp_string *fst, *snd; \
		\
		type_check(car(args),  SEXP_STRING, env); \
		type_check(cadr(args), SEXP_STRING, env); \
		\
		fst = sexp_string(car(args)); \
		snd = sexp_string(cadr(args)); \
		\
		if (fst->size != snd->size) \
			return make_bool(false); \
		\
		for (size_t i = 0; i < fst->size; i++) { \
			if (!(tolower(fst->data[i]) op tolower(snd->data[i]))) \
				return make_bool(false); \
		} \
		return make_bool(true); \
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
	sexp_t cons, sexp;
	struct sexp_string *str;
	size_t i = 0, size = 0, length = 0;

	/* count combined size/length */
	sexp_list_for_each(cons, args) {
		struct sexp_string *s = string_cast(car(cons), env);
		size += s->size;
		length += s->length;
	}

	/* allocate */
	sexp = make_string(size, size, length);
	str = sexp_string(sexp);

	/* copy */
	sexp_list_for_each(cons, args) {
		struct sexp_string *other = sexp_string(car(cons));
		for (size_t j = 0; j < other->size; j++)
			str->data[i++] = other->data[j];
	}
	return sexp;
}

DEFUN(scm_string_to_list, args, env)
{
	return string_to_list(type_check(car(args), SEXP_STRING, env));
}

DEFUN(scm_list_to_string, args, env)
{
	return list_to_string(type_check_list(car(args), env), env);
}

static sexp_t copy_to(sexp_t to, size_t at, sexp_t from, size_t start,
		size_t end)
{
	struct sexp_string *tos = sexp_string(to), *froms = sexp_string(from);
	for (size_t i = start; i < end; i++)
		tos->data[at++] = froms->data[i];
	return to;
}

DEFUN(scm_string_fill, args, env)
{
	sexp_t ch;
	long end, start;
	struct sexp_string *str;
	int nr_args = list_length(args);

	str = string_cast(car(args), env);
	ch = type_check(cadr(args), SEXP_CHAR, env);
	start = (nr_args > 2) ? fixnum_cast(caddr(args), env) : 0;
	end = (nr_args > 3) ? fixnum_cast(cadddr(args), env) : (long) str->size;

	check_copy(str->size, start, end, env);

	for (size_t i = start; i < (size_t) end; i++)
		str->data[i] = sexp_char(ch);

	return unspecified();
}

sexp_t string_copy(sexp_t string)
{
	struct sexp_string *from_str = sexp_string(string);
	sexp_t to = make_string(from_str->size, from_str->size, from_str->size);
	struct sexp_string *to_str = sexp_string(to);

	for (size_t i = 0; i < from_str->size; i++)
		to_str->data[i] = from_str->data[i];

	return to;
}

DEFUN(scm_string_copy, args, env)
{
	sexp_t from;
	long start, end;
	struct sexp_string *str;
	int nr_args = list_length(args);

	from = type_check(car(args), SEXP_STRING, env);
	str = sexp_string(from);
	start = (nr_args > 1) ? fixnum_cast(cadr(args), env) : 0;
	end = (nr_args > 2) ? fixnum_cast(caddr(args), env) : (long) str->size;

	check_copy(str->size, start, end, env);

	return copy_to(make_string(end - start, end - start, end - start), 0,
			from, start, end);
}

DEFUN(scm_string_copy_to, args, env)
{
	sexp_t to, from;
	long at, start, end;
	struct sexp_string *from_str, *to_str;
	int nr_args = list_length(args);

	to = type_check(car(args), SEXP_STRING, env);
	at = fixnum_cast(cadr(args), env);
	from = type_check(caddr(args), SEXP_STRING, env);
	to_str = sexp_string(to);
	from_str = sexp_string(from);
	start = (nr_args > 3) ? fixnum_cast(cadddr(args), env) : 0;
	end = (nr_args > 4) ? fixnum_cast(caddddr(args), env)
		: (long) from_str->size;

	check_copy_to(to_str->size, at, from_str->size, start, end, env);

	copy_to(to, at, from, start, end);
	return unspecified();
}
