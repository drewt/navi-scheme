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
	size_t size = 0;

	sexp_list_for_each(cons, list) {
		_type_check(car(cons), SEXP_CHAR, env);
		size += u_char_size(sexp_char(car(cons)));
	}

	int i = 0;
	sexp_t sexp = make_string(size);
	struct sexp_bytevec *vec = sexp_bytevec(sexp);

	sexp_list_for_each(cons, list) {
		u_set_char((char*)vec->data, &i, sexp_char(car(cons)));
	}
	return sexp;
}

char *scm_to_c_string(sexp_t string)
{
	struct sexp_bytevec *vec = sexp_bytevec(string);
	char *cstr = malloc(vec->size + 1);

	for (size_t i = 0; i < vec->size; i++)
		cstr[i] = vec->data[i];
	cstr[vec->size] = '\0';
	return cstr;
}

size_t string_length(sexp_t string)
{
	return u_strlen((char*)sexp_bytevec(string)->data);
}

DEFUN(scm_stringp, args)
{
	return make_bool(sexp_type(car(args)) == SEXP_STRING);
}

DEFUN(scm_make_string, args)
{
	uchar ch = ' ';
	int nr_args = list_length(args);

	long length = fixnum_cast(car(args));

	if (length < 0)
		error(____env, "invalid length");
	if (nr_args > 1)
		ch = char_cast(cadr(args));

	/* FIXME: invalid codepoint? */
	sexp_t string = make_string(length * u_char_size(ch));

	int j = 0;
	struct sexp_bytevec *vec = sexp_bytevec(string);
	for (size_t i = 0; i < (size_t) length; i++)
		u_set_char_raw((char*)vec->data, &j, ch);

	return string;
}

DEFUN(scm_string, args)
{
	return list_to_string(args, ____env);
}

DEFUN(scm_string_length, args)
{
	type_check(car(args), SEXP_STRING);
	return make_num(u_strlen((char*)sexp_bytevec(car(args))->data));
}

DEFUN(scm_string_ref, args)
{
	long k;
	struct sexp_bytevec *vec;

	type_check(car(args), SEXP_STRING);
	type_check(cadr(args), SEXP_NUM);

	vec = sexp_bytevec(car(args));
	k = sexp_num(cadr(args));

	if (k < 0 || (size_t) k >= vec->size)
		die("string index out of bounds");

	return make_char(vec->data[k]);
}

DEFUN(scm_string_set, args)
{
	long k;
	struct sexp_bytevec *vec;

	type_check(car(args),   SEXP_STRING);
	type_check(cadr(args),  SEXP_NUM);
	type_check(caddr(args), SEXP_CHAR);

	vec = sexp_bytevec(car(args));
	k = sexp_num(cadr(args));

	if (k < 0 || (size_t) k >= vec->size)
		die("string index out of bounds");

	vec->data[k] = sexp_char(caddr(args));
	return unspecified();
}

#define BINARY_PREDICATE(cname, op) \
	DEFUN(cname, args) \
	{ \
		struct sexp_bytevec *fst, *snd; \
		\
		type_check(car(args),  SEXP_STRING); \
		type_check(cadr(args), SEXP_STRING); \
		\
		fst = sexp_bytevec(car(args)); \
		snd = sexp_bytevec(cadr(args)); \
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
	DEFUN(cname, args) \
	{ \
		struct sexp_bytevec *fst, *snd; \
		\
		type_check(car(args),  SEXP_STRING); \
		type_check(cadr(args), SEXP_STRING); \
		\
		fst = sexp_bytevec(car(args)); \
		snd = sexp_bytevec(cadr(args)); \
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

DEFUN(scm_substring, args)
{
	return CALL(scm_string_copy, args);
}

DEFUN(scm_string_append, args)
{
	sexp_t cons, sexp;
	struct sexp_bytevec *vec;
	size_t i = 0, size = 0;

	/* count combined size */
	sexp_list_for_each(cons, args) {
		size += bytevec_cast(car(cons), SEXP_STRING)->size;
	}

	/* allocate */
	sexp = make_string(size);
	vec = sexp_bytevec(sexp);

	/* copy */
	sexp_list_for_each(cons, args) {
		struct sexp_bytevec *other = sexp_bytevec(car(cons));
		for (size_t j = 0; j < other->size; j++)
			vec->data[i++] = other->data[j];
	}
	return sexp;
}

DEFUN(scm_string_to_list, args)
{
	return bytevec_to_list(type_check(car(args), SEXP_STRING));
}

DEFUN(scm_list_to_string, args)
{
	return list_to_string(type_check_list(car(args)), ____env);
}

static bool copy_valid(size_t to, long at, size_t from, long start, long end)
{
	if (at < 0 || (size_t) at >= to)
		return false;

	if (start < 0 || (size_t) start >= from)
		return false;

	if (end < start || (size_t) end > from)
		return false;

	if (to - at < (size_t) (end - start))
		return false;

	return true;
}

static bool indices_valid(size_t size, long start, long end)
{
	return start >= 0 && end >= 0 && end > start && (size_t) (end - start) <= size;
}

static sexp_t copy_to(sexp_t to, size_t at, sexp_t from, size_t start,
		size_t end)
{
	struct sexp_bytevec *tov = sexp_bytevec(to), *fromv = sexp_bytevec(from);
	for (size_t i = 0; i < end; i++)
		tov->data[at++] = fromv->data[i];
	return to;
}

DEFUN(scm_string_fill, args)
{
	sexp_t ch;
	long end, start;
	struct sexp_bytevec *vec;
	int nr_args = list_length(args);

	vec = bytevec_cast(car(args), SEXP_STRING);
	ch = type_check(cadr(args), SEXP_CHAR);
	start = (nr_args > 2) ? fixnum_cast(caddr(args)) : 0;
	end = (nr_args > 3) ? fixnum_cast(cadddr(args)) : (long) vec->size;

	if (!indices_valid(vec->size, start, end))
		die("invalid indices");

	for (size_t i = start; i < (size_t) end; i++)
		vec->data[i] = sexp_char(ch);

	return unspecified();
}

sexp_t string_copy(sexp_t string)
{
	struct sexp_bytevec *from_vec = sexp_bytevec(string);
	sexp_t to = make_string(from_vec->size);
	struct sexp_bytevec *to_vec = sexp_bytevec(to);

	for (size_t i = 0; i < from_vec->size; i++)
		to_vec->data[i] = from_vec->data[i];

	return to;
}

DEFUN(scm_string_copy, args)
{
	sexp_t from;
	long start, end;
	struct sexp_bytevec *vec;
	int nr_args = list_length(args);

	from = type_check(car(args), SEXP_STRING);
	vec = sexp_bytevec(from);
	start = (nr_args > 1) ? fixnum_cast(cadr(args)) : 0;
	end = (nr_args > 2) ? fixnum_cast(caddr(args)) : (long) vec->size;

	if (!indices_valid(vec->size, start, end))
		die("invalid indices");

	return copy_to(make_string(end - start), 0, from, start, end);
}

DEFUN(scm_string_copy_to, args)
{
	sexp_t to, from;
	long at, start, end;
	struct sexp_bytevec *from_vec, *to_vec;
	int nr_args = list_length(args);

	to = type_check(car(args), SEXP_STRING);
	at = fixnum_cast(cadr(args));
	from = type_check(caddr(args), SEXP_STRING);
	to_vec = sexp_bytevec(to);
	from_vec = sexp_bytevec(from);
	start = (nr_args > 3) ? fixnum_cast(cadddr(args)) : 0;
	end = (nr_args > 4) ? fixnum_cast(caddddr(args)) : (long) from_vec->size;

	if (!copy_valid(to_vec->size, at, from_vec->size, start, end))
		die("invalid indices");

	copy_to(to, at, from, start, end);
	return unspecified();
}
