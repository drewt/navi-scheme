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
		ptr = navi_pair(ptr->cdr);
		ptr->car = vector->data[i];
	}
	ptr->cdr = navi_make_nil();
	return head.cdr;
}

DEFUN(vectorp, "vector?", 1, 0, NAVI_ANY)
{
	return navi_make_bool(navi_type(scm_arg1) == NAVI_VECTOR);
}

DEFUN(make_vector, "make-vector", 1, NAVI_PROC_VARIADIC, NAVI_FIXNUM)
{
	navi_obj vec;

	if (scm_nr_args > 2)
		navi_arity_error(scm_env, navi_make_symbol("make-vector"));

	vec = navi_make_vector(navi_fixnum(scm_arg1));
	navi_vector_fill(vec, (scm_nr_args < 2) ? navi_make_bool(false) : scm_arg2);
	return vec;
}

DEFUN(vector, "vector", 0, NAVI_PROC_VARIADIC)
{
	return navi_list_to_vector(scm_args);
}

DEFUN(vector_length, "vector-length", 1, 0, NAVI_VECTOR)
{
	return navi_make_fixnum(navi_vector(scm_arg1)->size);
}

DEFUN(vector_ref, "vector-ref", 2, 0, NAVI_VECTOR, NAVI_FIXNUM)
{
	if (navi_fixnum(scm_arg2) >= (long) navi_vector(scm_arg1)->size)
		navi_error(scm_env, "vector index out of bounds");

	return navi_vector_ref(scm_arg1, navi_fixnum(scm_arg2));
}

DEFUN(vector_set, "vector-set!", 3, 0, NAVI_VECTOR, NAVI_FIXNUM, NAVI_ANY)
{
	if (navi_fixnum(scm_arg2) >= (long) navi_vector(scm_arg1)->size)
		navi_error(scm_env, "vector index out of bounds");

	navi_vector(scm_arg1)->data[navi_fixnum(scm_arg2)] = scm_arg3;
	return navi_unspecified();
}

DEFUN(vector_to_list, "vector->list", 1, 0, NAVI_VECTOR)
{
	return navi_vector_to_list(scm_arg1);
}

DEFUN(list_to_vector, "list->vector", 1, 0, NAVI_LIST)
{
	return navi_list_to_vector(scm_arg1);
}

static navi_obj copy_to(navi_obj to, size_t at, navi_obj from, size_t start,
		size_t end)
{
	struct navi_vector *tov = navi_vector(to), *fromv = navi_vector(from);
	for (size_t i = start; i < end; i++)
		tov->data[at++] = fromv->data[i];
	return to;
}

DEFUN(vector_fill, "vector-fill!", 2, NAVI_PROC_VARIADIC, NAVI_VECTOR, NAVI_ANY)
{
	navi_obj fill;
	long end, start;
	struct navi_vector *vec;

	vec = navi_vector(scm_arg1);
	fill = scm_arg2;
	start = (scm_nr_args > 2) ? navi_fixnum_cast(scm_arg3, scm_env) : 0;
	end = (scm_nr_args > 3) ? navi_fixnum_cast(scm_arg4, scm_env) : (long) vec->size;

	navi_check_copy(vec->size, start, end, scm_env);

	for (size_t i = start; i < (size_t) end; i++)
		vec->data[i] = fill;

	return navi_unspecified();
}

DEFUN(vector_copy, "vector-copy", 1, NAVI_PROC_VARIADIC, NAVI_VECTOR)
{
	navi_obj from;
	long start, end;
	struct navi_vector *vec;

	from = scm_arg1;
	vec = navi_vector(from);
	start = (scm_nr_args > 1) ? navi_fixnum_cast(scm_arg2, scm_env) : 0;
	end = (scm_nr_args > 2) ? navi_fixnum_cast(scm_arg3, scm_env) : (long) vec->size;

	navi_check_copy(vec->size, start, end, scm_env);

	return copy_to(navi_make_vector(end-start), 0, from, start, end);
}

DEFUN(vector_copy_to, "vector-copy!", 3, NAVI_PROC_VARIADIC,
		NAVI_VECTOR, NAVI_FIXNUM, NAVI_VECTOR)
{
	navi_obj to, from;
	long at, start, end;
	struct navi_vector *fromv, *tov;

	to = scm_arg1;
	at = navi_fixnum(scm_arg2);
	from = scm_arg3;
	tov = navi_vector(to);
	fromv = navi_vector(from);
	start = (scm_nr_args > 3) ? navi_fixnum_cast(scm_arg4, scm_env) : 0;
	end = (scm_nr_args > 4) ? navi_fixnum_cast(scm_arg5, scm_env) : (long) fromv->size;

	navi_check_copy_to(tov->size, at, fromv->size, start, end, scm_env);

	copy_to(to, at, from, start, end);
	return navi_unspecified();
}

navi_obj navi_vector_map(navi_obj proc, navi_obj to, navi_obj from, navi_env env)
{
	struct navi_vector *tov = navi_vector_cast(to, env);
	struct navi_vector *fromv = navi_vector_cast(from, env);

	for (size_t i = 0; i < tov->size; i++) {
		navi_obj call = navi_list(proc, fromv->data[i], navi_make_void());
		tov->data[i] = navi_eval(call, env);
	}
	return to;
}

DEFUN(vector_map_ip, "vector-map!", 2, 0, NAVI_PROCEDURE, NAVI_VECTOR)
{
	navi_check_arity(scm_arg1, 1, scm_env);
	return navi_vector_map(scm_arg1, scm_arg2, scm_arg2, scm_env);
}

static navi_obj arg_list(struct navi_vector *vecs[], size_t nr_vecs, size_t k)
{
	navi_obj args, cons;
	cons = args = navi_make_pair(navi_make_nil(), navi_make_nil());
	for (size_t i = 0; i < nr_vecs; i++) {
		navi_obj cdr = navi_make_pair(vecs[i]->data[k], navi_make_nil());
		navi_set_cdr(cons, cdr);
		cons = navi_cdr(cons);
	}
	return navi_cdr(args);
}

static navi_obj do_apply(navi_obj proc, navi_obj args, navi_env env)
{
	return navi_force_tail(navi_apply(navi_procedure(proc), args, env), env);
}

static size_t vector_array_fill(struct navi_vector *array[], navi_obj args,
		navi_env env)
{
	navi_obj cons;
	size_t i = 0, min_len = navi_vector(navi_car(args))->size;
	navi_list_for_each(cons, args) {
		array[i] = navi_vector_cast(navi_car(cons), env);
		min_len = array[i]->size < min_len ? array[i]->size : min_len;
		i++;
	}
	return min_len;
}

DEFUN(vector_for_each, "vector-for-each", 2, NAVI_PROC_VARIADIC,
		NAVI_PROCEDURE, NAVI_VECTOR)
{
	size_t min_len;
	struct navi_vector *args[scm_nr_args-1];

	navi_check_arity(scm_arg1, scm_nr_args-1, scm_env);
	min_len = vector_array_fill(args, navi_cdr(scm_args), scm_env);

	for (size_t i = 0; i < min_len; i++)
		do_apply(scm_arg1, arg_list(args, scm_nr_args-1, i), scm_env);
	return navi_unspecified();
}

DEFUN(vector_map, "vector-map", 2, NAVI_PROC_VARIADIC,
		NAVI_PROCEDURE, NAVI_VECTOR)
{
	size_t min_len;
	navi_obj result;
	struct navi_vector *vec;
	struct navi_vector *args[scm_nr_args-1];

	navi_check_arity(scm_arg1, scm_nr_args-1, scm_env);
	min_len = vector_array_fill(args, navi_cdr(scm_args), scm_env);

	result = navi_make_vector(min_len);
	vec = navi_vector(result);

	for (size_t i = 0; i < min_len; i++) {
		vec->data[i] = do_apply(scm_arg1,
				arg_list(args, scm_nr_args-1, i), scm_env);
	}
	return result;
}
