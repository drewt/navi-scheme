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

#include "sexp.h"

void bytevec_fill(sexp_t vector, unsigned char fill)
{
	struct sexp_bytevec *vec = sexp_bytevec(vector);

	for (size_t i = 0; i < vec->size; i++)
		vec->data[i] = fill;
}

sexp_t list_to_bytevec(sexp_t list, env_t env)
{
	sexp_t cons, vec = make_bytevec(list_length(list));

	unsigned i = 0;
	struct sexp_bytevec *vector = sexp_bytevec(vec);
	sexp_list_for_each(cons, list) {
		_type_check_byte(car(cons), env);
		vector->data[i++] = sexp_num(car(cons));
	}
	return vec;
}

sexp_t bytevec_to_list(sexp_t sexp)
{
	struct sexp_bytevec *vector;
	struct sexp_pair head, *ptr = &head;

	vector = sexp_bytevec(sexp);
	for (size_t i = 0; i < vector->size; i++) {
		ptr->cdr = make_empty_pair();
		ptr = &ptr->cdr.p->data->pair;
		ptr->car = make_char(vector->data[i]);
	}
	ptr->cdr = make_nil();
	return head.cdr;
}

/* FIXME: assumes ASCII */
sexp_t string_to_bytevec(sexp_t string)
{
	struct sexp_vector *svec = sexp_vector(string);
	sexp_t sexp = make_bytevec(svec->size);
	struct sexp_bytevec *bvec = sexp_bytevec(sexp);

	for (size_t i = 0; i < svec->size; i++)
		bvec->data[i] = sexp_char(svec->data[i]);

	return sexp;
}

/* FIXME: assumes ASCII */
sexp_t bytevec_to_string(sexp_t bytevec)
{
	struct sexp_bytevec *bvec = sexp_bytevec(bytevec);
	sexp_t sexp = make_string(bvec->size, bvec->size);
	struct sexp_vector *svec = sexp_vector(sexp);

	for (size_t i = 0; i < bvec->size; i++)
		svec->data[i] = make_char(bvec->data[i]);

	return sexp;
}

char *bytevec_to_c_string(sexp_t sexp)
{
	struct sexp_bytevec *vec = sexp_bytevec(sexp);
	char *cstr = malloc(vec->size + 1);

	for (size_t i = 0; i < vec->size; i++)
		cstr[i] = vec->data[i];
	cstr[vec->size] = '\0';
	return cstr;
}

DEFUN(scm_make_bytevector, args)
{
	sexp_t sexp;
	int nr_args = list_length(args);

	if (nr_args != 1 && nr_args != 2)
		error(____env, "wrong number of arguments");

	type_check(car(args), SEXP_NUM);

	sexp = make_bytevec(sexp_num(car(args)));
	if (nr_args == 2) {
		type_check_byte(cadr(args));
		bytevec_fill(sexp, sexp_num(cadr(args)));
	}
	return sexp;
}

DEFUN(scm_bytevector, args)
{
	return list_to_bytevec(args, ____env);
}

DEFUN(scm_bytevector_length, args)
{
	type_check(car(args), SEXP_BYTEVEC);
	return make_num(sexp_bytevec(car(args))->size);
}

DEFUN(scm_bytevector_u8_ref, args)
{
	type_check(car(args),  SEXP_BYTEVEC);
	type_check_range(cadr(args), 0, sexp_bytevec(car(args))->size);

	return bytevec_ref(car(args), sexp_num(cadr(args)));
}

DEFUN(scm_bytevector_u8_set, args)
{
	type_check(car(args), SEXP_BYTEVEC);
	type_check_range(cadr(args), 0, sexp_bytevec(car(args))->size);
	type_check_byte(caddr(args));

	sexp_bytevec(car(args))->data[sexp_num(cadr(args))] = sexp_num(caddr(args));
	return unspecified();
}

DEFUN(scm_bytevector_append, args)
{
	sexp_t cons, sexp;
	struct sexp_bytevec *vec;
	size_t i = 0, size = 0;

	sexp_list_for_each(cons, args) {
		size += bytevec_cast(car(cons), SEXP_BYTEVEC)->size;
	}

	sexp = make_bytevec(size);
	vec = sexp_bytevec(sexp);

	sexp_list_for_each(cons, args) {
		struct sexp_bytevec *other = sexp_bytevec(car(cons));
		for (size_t j = 0; j < other->size; j++)
			vec->data[i++] = other->data[j];
	}
	vec->data[i] = '\0';
	return sexp;
}

static bool indices_valid(size_t size, long start, long end)
{
	return start >= 0 && end >= 0 && end > start && (size_t) (end - start) > size;
}

static sexp_t copy_to(sexp_t to, size_t at, sexp_t from, size_t start,
		size_t end)
{
	for (size_t i = start; i < end; i++)
		sexp_bytevec(to)->data[at++] = sexp_bytevec(from)->data[i];
	return to;
}

DEFUN(scm_bytevector_copy, args)
{
	sexp_t from;
	long start, end;
	struct sexp_bytevec *vec;
	int nr_args = list_length(args);

	from = type_check(car(args), SEXP_BYTEVEC);
	vec = sexp_bytevec(from);
	start = (nr_args > 1) ? fixnum_cast(cadr(args)) : 0;
	end = (nr_args > 2) ? fixnum_cast(caddr(args)) : (long) vec->size;

	if (!indices_valid(vec->size, start, end))
		error(____env, "invalid indices");

	return copy_to(make_bytevec(end - start), 0, from, start, end);
}

/*DEFUN(scm_bytevector_copy_, args)
{
	sexp_t to;
	long end, start = 0;
	int nr_args = list_length(args);

	type_check(car(args), SEXP_BYTEVEC);

	if (nr_args > 1) {
		type_check(cadr(args), SEXP_NUM);
		start = sexp_num(cadr(args));
	}

	if (nr_args > 2) {
		type_check(caddr(args), SEXP_NUM);
		end = sexp_num(caddr(args));
	} else {
		end = sexp_string_size(car(args));
	}

	if (end < start)
		die("invalid arguments");

	to = make_empty_string(end - start);
	to.p->type = SEXP_BYTEVEC;

	if (!bytevec_copy_valid(to, 0, car(args), start, end))
		die("invalid arguments");

	bytevec_copy_to(to, 0, car(args), start, end);
	return to;
}*/

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

DEFUN(scm_bytevector_copy_to, args)
{
	sexp_t to, from;
	long at, start, end;
	struct sexp_bytevec *from_vec, *to_vec;
	int nr_args = list_length(args);

	to = type_check(car(args), SEXP_BYTEVEC);
	at = fixnum_cast(cadr(args));
	from = type_check(caddr(args), SEXP_BYTEVEC);
	to_vec = sexp_bytevec(to);
	from_vec = sexp_bytevec(from);
	start = (nr_args > 3) ? fixnum_cast(cadddr(args)) : 0;
	end = (nr_args > 4) ? fixnum_cast(caddddr(args)) : (long)from_vec->size;

	if (!copy_valid(to_vec->size, at, from_vec->size, start, end))
		error(____env, "invalid indices");

	copy_to(to, at, from, start, end);
	return unspecified();
}

/*DEFUN(scm_bytevector_copy_to, args)
{
	sexp_t to, from;
	long at, start = 0, end;
	int nr_args = list_length(args);

	type_check(car(args),   SEXP_BYTEVEC);
	type_check(cadr(args),  SEXP_NUM);
	type_check(caddr(args), SEXP_BYTEVEC);

	to = car(args);
	at = sexp_num(cadr(args));
	from = caddr(args);

	if (nr_args > 3) {
		type_check(cadddr(args), SEXP_NUM);
		start = sexp_num(cadddr(args));
	}
	
	if (nr_args > 4) {
		type_check(caddddr(args), SEXP_NUM);
		end = sexp_num(caddddr(args));
	} else {
		end = sexp_string_size(from);
	}

	if (!bytevec_copy_valid(to, at, from, start, end))
		die("invalid arguments");

	bytevec_copy_to(to, at, from, start, end);
	return unspecified();
}*/
