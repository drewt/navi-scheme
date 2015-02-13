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

void navi_vector_fill(navi_obj vector, navi_obj fill)
{
	struct navi_vector *vec = navi_vector(vector);

	for (size_t i = 0; i < vec->size; i++)
		vec->data[i] = fill;
}

navi_obj navi_list_to_vector(navi_obj list)
{
	navi_obj cons, vec = navi_make_vector(navi_list_length(list));

	unsigned i = 0;
	struct navi_vector *vector = navi_vector(vec);
	navi_list_for_each(cons, list) {
		vector->data[i++] = navi_car(cons);
	}
	return vec;
}

navi_obj navi_vector_to_list(navi_obj obj)
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

DEFUN(vectorp, args, env, "vector?", 1, 0, NAVI_ANY)
{
	return navi_make_bool(navi_type(navi_car(args)) == NAVI_VECTOR);
}

DEFUN(make_vector, args, env, "make-vector", 1, NAVI_PROC_VARIADIC,
		NAVI_NUM)
{
	navi_obj vec;
	int nr_args = navi_list_length(args);

	if (nr_args > 2)
		navi_arity_error(env, navi_make_symbol("make-vector"));

	vec = navi_make_vector(navi_num(navi_car(args)));
	navi_vector_fill(vec, (nr_args < 2) ? navi_make_bool(false) : navi_cadr(args));
	return vec;
}

DEFUN(vector, args, env, "vector", 0, NAVI_PROC_VARIADIC)
{
	return navi_list_to_vector(args);
}

DEFUN(vector_length, args, env, "vector-length", 1, 0, NAVI_VECTOR)
{
	return navi_make_num(navi_vector(navi_car(args))->size);
}

DEFUN(vector_ref, args, env, "vector-ref", 2, 0, NAVI_VECTOR, NAVI_NUM)
{
	if (navi_num(navi_cadr(args)) >= (long) navi_vector(navi_car(args))->size)
		navi_error(env, "vector index out of bounds");

	return navi_vector_ref(navi_car(args), navi_num(navi_cadr(args)));
}

DEFUN(vector_set, args, env, "vector-set!", 3, 0, NAVI_VECTOR, NAVI_NUM, NAVI_ANY)
{
	if (navi_num(navi_cadr(args)) >= (long) navi_vector(navi_car(args))->size)
		navi_error(env, "vector index out of bounds");

	navi_vector(navi_car(args))->data[navi_num(navi_cadr(args))] = navi_caddr(args);
	return navi_unspecified();
}

DEFUN(vector_to_list, args, env, "vector->list", 1, 0, NAVI_VECTOR)
{
	return navi_vector_to_list(navi_car(args));
}

DEFUN(list_to_vector, args, env, "list->vector", 1, 0, NAVI_LIST)
{
	return navi_list_to_vector(navi_car(args));
}

static navi_obj copy_to(navi_obj to, size_t at, navi_obj from, size_t start,
		size_t end)
{
	struct navi_vector *tov = navi_vector(to), *fromv = navi_vector(from);
	for (size_t i = start; i < end; i++)
		tov->data[at++] = fromv->data[i];
	return to;
}

DEFUN(vector_fill, args, env, "vector-fill!", 2, NAVI_PROC_VARIADIC,
		NAVI_VECTOR, NAVI_ANY)
{
	navi_obj fill;
	long end, start;
	struct navi_vector *vec;
	int nr_args = navi_list_length(args);

	vec = navi_vector(navi_car(args));
	fill = navi_cadr(args);
	start = (nr_args > 2) ? navi_fixnum_cast(navi_caddr(args), env) : 0;
	end = (nr_args > 3) ? navi_fixnum_cast(navi_cadddr(args), env) : (long) vec->size;

	navi_check_copy(vec->size, start, end, env);

	for (size_t i = start; i < (size_t) end; i++)
		vec->data[i] = fill;

	return navi_unspecified();
}

DEFUN(vector_copy, args, env, "vector-copy", 1, NAVI_PROC_VARIADIC, NAVI_VECTOR)
{
	navi_obj from;
	long start, end;
	struct navi_vector *vec;
	int nr_args = navi_list_length(args);

	from = navi_car(args);
	vec = navi_vector(from);
	start = (nr_args > 1) ? navi_fixnum_cast(navi_cadr(args), env) : 0;
	end = (nr_args > 2) ? navi_fixnum_cast(navi_caddr(args), env) : (long) vec->size;

	navi_check_copy(vec->size, start, end, env);

	return copy_to(navi_make_vector(end-start), 0, from, start, end);
}

DEFUN(vector_copy_to, args, env, "vector-copy!", 3, NAVI_PROC_VARIADIC,
		NAVI_VECTOR, NAVI_NUM, NAVI_VECTOR)
{
	navi_obj to, from;
	long at, start, end;
	struct navi_vector *fromv, *tov;
	int nr_args = navi_list_length(args);

	to = navi_car(args);
	at = navi_num(navi_cadr(args));
	from = navi_caddr(args);
	tov = navi_vector(to);
	fromv = navi_vector(from);
	start = (nr_args > 3) ? navi_fixnum_cast(navi_cadddr(args), env) : 0;
	end = (nr_args > 4) ? navi_fixnum_cast(navi_caddddr(args), env)
		: (long) fromv->size;

	navi_check_copy_to(tov->size, at, fromv->size, start, end, env);

	copy_to(to, at, from, start, end);
	return navi_unspecified();
}

navi_obj navi_vector_map(navi_obj proc, navi_obj to, navi_obj from, navi_env env)
{
	struct navi_vector *tov = navi_vector_cast(to, NAVI_VECTOR, env);
	struct navi_vector *fromv = navi_vector_cast(from, NAVI_VECTOR, env);

	for (size_t i = 0; i < tov->size; i++) {
		navi_obj call = navi_list(proc, fromv->data[i], navi_make_void());
		tov->data[i] = navi_eval(call, env);
	}
	return to;
}

DEFUN(vector_map_ip, args, env, "vector-map!", 2, 0, NAVI_PROCEDURE, NAVI_VECTOR)
{
	navi_check_arity(navi_car(args), 1, env);
	return navi_vector_map(navi_car(args), navi_cadr(args), navi_cadr(args), env);
}

DEFUN(vector_map, args, env, "vector-map", 2, 0, NAVI_PROCEDURE, NAVI_VECTOR)
{
	navi_check_arity(navi_car(args), 1, env);
	return navi_vector_map(navi_car(args),
			navi_make_vector(navi_vector_length(navi_cadr(args))),
			navi_cadr(args), env);
}
