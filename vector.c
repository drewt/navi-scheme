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

#include "navi.h"

void navi_vector_fill(navi_t vector, navi_t fill)
{
	struct navi_vector *vec = navi_vector(vector);

	for (size_t i = 0; i < vec->size; i++)
		vec->data[i] = fill;
}

navi_t navi_list_to_vector(navi_t list)
{
	navi_t cons, vec = navi_make_vector(navi_list_length(list));

	unsigned i = 0;
	struct navi_vector *vector = navi_vector(vec);
	navi_list_for_each(cons, list) {
		vector->data[i++] = navi_car(cons);
	}
	return vec;
}

navi_t navi_vector_to_list(navi_t obj)
{
	struct navi_vector *vector;
	struct navi_pair head, *ptr = &head;

	vector = navi_vector(obj);
	for (size_t i = 0; i < vector->size; i++) {
		ptr->cdr = navi_make_empty_pair();
		ptr = &ptr->cdr.p->data->pair;
		ptr->car = vector->data[i];
	}
	ptr->cdr = navi_make_nil();
	return head.cdr;
}

DEFUN(scm_vectorp, args, env)
{
	return navi_make_bool(navi_type(navi_car(args)) == NAVI_VECTOR);
}

DEFUN(scm_make_vector, args, env)
{
	navi_t vec;
	int nr_args = navi_list_length(args);

	if (nr_args != 1 && nr_args != 2)
		navi_arity_error(env, navi_make_symbol("make-vector"));

	navi_type_check(navi_car(args), NAVI_NUM, env);
	vec = navi_make_vector(navi_num(navi_car(args)));
	navi_vector_fill(vec, (nr_args < 2) ? navi_make_bool(false) : navi_cadr(args));
	return vec;
}

DEFUN(scm_vector, args, env)
{
	return navi_list_to_vector(args);
}

DEFUN(scm_vector_length, args, env)
{
	navi_type_check(navi_car(args), NAVI_VECTOR, env);
	return navi_make_num(navi_vector(navi_car(args))->size);
}

DEFUN(scm_vector_ref, args, env)
{
	navi_type_check(navi_car(args), NAVI_VECTOR, env);
	navi_type_check(navi_cadr(args), NAVI_NUM, env);

	if (navi_num(navi_cadr(args)) >= (long) navi_vector(navi_car(args))->size)
		navi_error(env, "vector index out of bounds");

	return navi_vector_ref(navi_car(args), navi_num(navi_cadr(args)));
}

DEFUN(scm_vector_set, args, env)
{
	navi_type_check(navi_car(args), NAVI_VECTOR, env);
	navi_type_check(navi_cadr(args), NAVI_NUM, env);

	if (navi_num(navi_cadr(args)) >= (long) navi_vector(navi_car(args))->size)
		navi_error(env, "vector index out of bounds");

	navi_vector(navi_car(args))->data[navi_num(navi_cadr(args))] = navi_caddr(args);
	return navi_unspecified();
}

DEFUN(scm_vector_to_list, args, env)
{
	navi_type_check(navi_car(args), NAVI_VECTOR, env);
	return navi_vector_to_list(navi_car(args));
}

DEFUN(scm_list_to_vector, args, env)
{
	navi_type_check_list(navi_car(args), env);
	return navi_list_to_vector(navi_car(args));
}

static navi_t copy_to(navi_t to, size_t at, navi_t from, size_t start,
		size_t end)
{
	struct navi_vector *tov = navi_vector(to), *fromv = navi_vector(from);
	for (size_t i = start; i < end; i++)
		tov->data[at++] = fromv->data[i];
	return to;
}

DEFUN(scm_vector_fill, args, env)
{
	navi_t fill;
	long end, start;
	struct navi_vector *vec;
	int nr_args = navi_list_length(args);

	vec = navi_vector_cast(navi_car(args), NAVI_VECTOR, env);
	fill = navi_cadr(args);
	start = (nr_args > 2) ? navi_fixnum_cast(navi_caddr(args), env) : 0;
	end = (nr_args > 3) ? navi_fixnum_cast(navi_cadddr(args), env) : (long) vec->size;

	navi_check_copy(vec->size, start, end, env);

	for (size_t i = start; i < (size_t) end; i++)
		vec->data[i] = fill;

	return navi_unspecified();
}

DEFUN(scm_vector_copy, args, env)
{
	navi_t from;
	long start, end;
	struct navi_vector *vec;
	int nr_args = navi_list_length(args);

	from = navi_type_check(navi_car(args), NAVI_VECTOR, env);
	vec = navi_vector(from);
	start = (nr_args > 1) ? navi_fixnum_cast(navi_cadr(args), env) : 0;
	end = (nr_args > 2) ? navi_fixnum_cast(navi_caddr(args), env) : (long) vec->size;

	navi_check_copy(vec->size, start, end, env);

	return copy_to(navi_make_vector(end-start), 0, from, start, end);
}

DEFUN(scm_vector_copy_to, args, env)
{
	navi_t to, from;
	long at, start, end;
	struct navi_vector *fromv, *tov;
	int nr_args = navi_list_length(args);

	to = navi_type_check(navi_car(args), NAVI_VECTOR, env);
	at = navi_fixnum_cast(navi_cadr(args), env);
	from = navi_type_check(navi_caddr(args), NAVI_VECTOR, env);
	tov = navi_vector(to);
	fromv = navi_vector(from);
	start = (nr_args > 3) ? navi_fixnum_cast(navi_cadddr(args), env) : 0;
	end = (nr_args > 4) ? navi_fixnum_cast(navi_caddddr(args), env)
		: (long) fromv->size;

	navi_check_copy_to(tov->size, at, fromv->size, start, end, env);

	copy_to(to, at, from, start, end);
	return navi_unspecified();
}

navi_t navi_vector_map(navi_t proc, navi_t to, navi_t from, navi_env_t env)
{
	struct navi_vector *tov = navi_vector_cast(to, NAVI_VECTOR, env);
	struct navi_vector *fromv = navi_vector_cast(from, NAVI_VECTOR, env);

	for (size_t i = 0; i < tov->size; i++) {
		navi_t call = navi_list(proc, fromv->data[i], navi_make_void());
		tov->data[i] = navi_eval(call, env);
	}
	return to;
}

DEFUN(scm_vector_map_ip, args, env)
{
	navi_type_check_proc(navi_car(args), 1, env);
	navi_type_check(navi_cadr(args), NAVI_VECTOR, env);
	return navi_vector_map(navi_car(args), navi_cadr(args), navi_cadr(args), env);
}

DEFUN(scm_vector_map, args, env)
{
	navi_type_check_proc(navi_car(args), 1, env);
	navi_type_check(navi_cadr(args), NAVI_VECTOR, env);
	return navi_vector_map(navi_car(args),
			navi_make_vector(navi_vector_length(navi_cadr(args))),
			navi_cadr(args), env);
}
