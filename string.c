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
		_type_check(car(cons), SEXP_CHAR, env);
		size += u_char_size(sexp_char(car(cons)));
		length++;
	}

	unsigned i = 0;
	sexp_t sexp = make_string(size, length);
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
	for (unsigned i = 0; i < string->size;) {
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
	char *cstr = malloc(string->size + 1);

	for (size_t i = 0; i < string->size; i++)
		cstr[i] = string->data[i];
	cstr[string->size] = '\0';
	return cstr;
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
	sexp_t sexp = make_string(length * u_char_size(ch), length);

	unsigned j = 0;
	struct sexp_string *string = sexp_string(sexp);
	for (size_t i = 0; i < (size_t) length; i++)
		u_set_char_raw((char*)string->data, &j, ch);

	return sexp;
}

DEFUN(scm_string, args)
{
	return list_to_string(args, ____env);
}

DEFUN(scm_string_length, args)
{
	return make_num(string_cast(car(args))->length);
}

DEFUN(scm_string_ref, args)
{
	unsigned i = 0;
	struct sexp_string *str = string_cast(car(args));	
	long k = type_check_range(cadr(args), 0, str->length);

	u_skip_chars(str->data, k, &i);
	return make_char(u_get_char(str->data, &i));
}

DEFUN(scm_string_set, args)
{
	long k;
	struct sexp_string *str;

	type_check(car(args),   SEXP_STRING);
	type_check(cadr(args),  SEXP_NUM);
	type_check(caddr(args), SEXP_CHAR);

	str = sexp_string(car(args));
	k = sexp_num(cadr(args));

	if (k < 0 || (size_t) k >= str->size)
		die("string index out of bounds");

	// FIXME: UTF-8
	str->data[k] = sexp_char(caddr(args));
	return unspecified();
}

#define BINARY_PREDICATE(cname, op) \
	DEFUN(cname, args) \
	{ \
		struct sexp_string *fst, *snd; \
		\
		type_check(car(args),  SEXP_STRING); \
		type_check(cadr(args), SEXP_STRING); \
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
	DEFUN(cname, args) \
	{ \
		struct sexp_string *fst, *snd; \
		\
		type_check(car(args),  SEXP_STRING); \
		type_check(cadr(args), SEXP_STRING); \
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

DEFUN(scm_substring, args)
{
	return CALL(scm_string_copy, args);
}

DEFUN(scm_string_append, args)
{
	sexp_t cons, sexp;
	struct sexp_string *str;
	size_t i = 0, size = 0, length = 0;

	/* count combined size/length */
	sexp_list_for_each(cons, args) {
		struct sexp_string *s = string_cast(car(cons));
		size += s->size;
		length += s->length;
	}

	/* allocate */
	sexp = make_string(size, length);
	str = sexp_string(sexp);

	/* copy */
	sexp_list_for_each(cons, args) {
		struct sexp_string *other = sexp_string(car(cons));
		for (size_t j = 0; j < other->size; j++)
			str->data[i++] = other->data[j];
	}
	return sexp;
}

DEFUN(scm_string_to_list, args)
{
	return string_to_list(type_check(car(args), SEXP_STRING));
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
	struct sexp_string *tos = sexp_string(to), *froms = sexp_string(from);
	for (size_t i = 0; i < end; i++)
		tos->data[at++] = froms->data[i];
	return to;
}

DEFUN(scm_string_fill, args)
{
	sexp_t ch;
	long end, start;
	struct sexp_string *str;
	int nr_args = list_length(args);

	str = string_cast(car(args));
	ch = type_check(cadr(args), SEXP_CHAR);
	start = (nr_args > 2) ? fixnum_cast(caddr(args)) : 0;
	end = (nr_args > 3) ? fixnum_cast(cadddr(args)) : (long) str->size;

	if (!indices_valid(str->size, start, end))
		die("invalid indices");

	for (size_t i = start; i < (size_t) end; i++)
		str->data[i] = sexp_char(ch);

	return unspecified();
}

sexp_t string_copy(sexp_t string)
{
	struct sexp_string *from_str = sexp_string(string);
	sexp_t to = make_string(from_str->size, from_str->size);
	struct sexp_string *to_str = sexp_string(to);

	for (size_t i = 0; i < from_str->size; i++)
		to_str->data[i] = from_str->data[i];

	return to;
}

DEFUN(scm_string_copy, args)
{
	sexp_t from;
	long start, end;
	struct sexp_string *str;
	int nr_args = list_length(args);

	from = type_check(car(args), SEXP_STRING);
	str = sexp_string(from);
	start = (nr_args > 1) ? fixnum_cast(cadr(args)) : 0;
	end = (nr_args > 2) ? fixnum_cast(caddr(args)) : (long) str->size;

	if (!indices_valid(str->size, start, end))
		die("invalid indices");

	return copy_to(make_string(end - start, end - start), 0, from, start, end);
}

DEFUN(scm_string_copy_to, args)
{
	sexp_t to, from;
	long at, start, end;
	struct sexp_string *from_str, *to_str;
	int nr_args = list_length(args);

	to = type_check(car(args), SEXP_STRING);
	at = fixnum_cast(cadr(args));
	from = type_check(caddr(args), SEXP_STRING);
	to_str = sexp_string(to);
	from_str = sexp_string(from);
	start = (nr_args > 3) ? fixnum_cast(cadddr(args)) : 0;
	end = (nr_args > 4) ? fixnum_cast(caddddr(args)) : (long) from_str->size;

	if (!copy_valid(to_str->size, at, from_str->size, start, end))
		die("invalid indices");

	copy_to(to, at, from, start, end);
	return unspecified();
}
