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

void vector_fill(sexp_t vector, sexp_t fill)
{
	struct sexp_vector *vec = sexp_vector(vector);

	for (size_t i = 0; i < vec->size; i++)
		vec->data[i] = fill;
}

sexp_t list_to_vector(sexp_t list)
{
	sexp_t cons, vec = make_vector(list_length(list));

	unsigned i = 0;
	struct sexp_vector *vector = sexp_vector(vec);
	sexp_list_for_each(cons, list) {
		vector->data[i++] = car(cons);
	}
	return vec;
}

sexp_t vector_to_list(sexp_t sexp)
{
	struct sexp_vector *vector;
	struct sexp_pair head, *ptr = &head;

	vector = sexp_vector(sexp);
	for (size_t i = 0; i < vector->size; i++) {
		ptr->cdr = make_empty_pair();
		ptr = &ptr->cdr.p->data->pair;
		ptr->car = vector->data[i];
	}
	ptr->cdr = make_nil();
	return head.cdr;
}

DEFUN(scm_vectorp, args)
{
	return make_bool(sexp_type(car(args)) == SEXP_VECTOR);
}

DEFUN(scm_make_vector, args)
{
	sexp_t sexp;
	int nr_args = list_length(args);

	if (nr_args != 1 && nr_args != 2)
		die("wrong number of arguments");

	type_check(car(args), SEXP_NUM);
	sexp = make_vector(sexp_num(car(args)));
	vector_fill(sexp, (nr_args < 2) ? make_bool(false) : cadr(args));
	return sexp;
}

DEFUN(scm_vector, args)
{
	return list_to_vector(args);
}

DEFUN(scm_vector_length, args)
{
	type_check(car(args), SEXP_VECTOR);
	return make_num(sexp_vector(car(args))->size);
}

DEFUN(scm_vector_ref, args)
{
	type_check(car(args),  SEXP_VECTOR);
	type_check(cadr(args), SEXP_NUM);

	if (sexp_num(cadr(args)) >= (long) sexp_vector(car(args))->size)
		die("vector index out of bounds");

	return vector_ref(car(args), sexp_num(cadr(args)));
}

DEFUN(scm_vector_set, args)
{
	type_check(car(args),  SEXP_VECTOR);
	type_check(cadr(args), SEXP_NUM);

	if (sexp_num(cadr(args)) >= (long) sexp_vector(car(args))->size)
		die("vector index out of bounds");

	sexp_vector(car(args))->data[sexp_num(cadr(args))] = caddr(args);
	return unspecified();
}

DEFUN(scm_vector_to_list, args)
{
	type_check(car(args), SEXP_VECTOR);
	return vector_to_list(car(args));
}

DEFUN(scm_list_to_vector, args)
{
	type_check_list(car(args));
	return list_to_vector(car(args));
}

static sexp_t copy_to(sexp_t to, size_t at, sexp_t from, size_t start,
		size_t end)
{
	struct sexp_vector *tov = sexp_vector(to), *fromv = sexp_vector(from);
	for (size_t i = start; i < end; i++)
		tov->data[at++] = fromv->data[i];
	return to;
}

DEFUN(_scm_vector_fill, args)
{
	type_check(car(args), SEXP_VECTOR);
	vector_fill(car(args), cadr(args));
	return unspecified();
}

DEFUN(scm_vector_fill, args)
{
	sexp_t fill;
	long end, start;
	struct sexp_vector *vec;
	int nr_args = list_length(args);

	vec = vector_cast(car(args), SEXP_VECTOR);
	fill = cadr(args);
	start = (nr_args > 2) ? fixnum_cast(caddr(args)) : 0;
	end = (nr_args > 3) ? fixnum_cast(cadddr(args)) : (long) vec->size;

	check_copy(vec->size, start, end);

	for (size_t i = start; i < (size_t) end; i++)
		vec->data[i] = fill;

	return unspecified();
}

DEFUN(scm_vector_copy, args)
{
	sexp_t from;
	long start, end;
	struct sexp_vector *vec;
	int nr_args = list_length(args);

	from = type_check(car(args), SEXP_VECTOR);
	vec = sexp_vector(from);
	start = (nr_args > 1) ? fixnum_cast(cadr(args)) : 0;
	end = (nr_args > 2) ? fixnum_cast(caddr(args)) : (long) vec->size;

	check_copy(vec->size, start, end);

	return copy_to(make_vector(end-start), 0, from, start, end);
}

DEFUN(scm_vector_copy_to, args)
{
	sexp_t to, from;
	long at, start, end;
	struct sexp_vector *fromv, *tov;
	int nr_args = list_length(args);

	to = type_check(car(args), SEXP_VECTOR);
	at = fixnum_cast(cadr(args));
	from = type_check(caddr(args), SEXP_VECTOR);
	tov = sexp_vector(to);
	fromv = sexp_vector(from);
	start = (nr_args > 3) ? fixnum_cast(cadddr(args)) : 0;
	end = (nr_args > 4) ? fixnum_cast(caddddr(args)) : (long) fromv->size;

	check_copy_to(tov->size, at, fromv->size, start, end);

	copy_to(to, at, from, start, end);
	return unspecified();
}
