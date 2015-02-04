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
#include "navi/uchar.h"

static void bytevec_fill(navi_t vector, unsigned char fill)
{
	struct navi_bytevec *vec = navi_bytevec(vector);

	for (size_t i = 0; i < vec->size; i++)
		vec->data[i] = fill;
}

navi_t navi_list_to_bytevec(navi_t list, navi_env_t env)
{
	navi_t cons, vec = navi_make_bytevec(navi_list_length(list));

	unsigned i = 0;
	struct navi_bytevec *vector = navi_bytevec(vec);
	navi_list_for_each(cons, list) {
		navi_type_check_byte(navi_car(cons), env);
		vector->data[i++] = navi_num(navi_car(cons));
	}
	return vec;
}

navi_t navi_bytevec_to_list(navi_t obj)
{
	struct navi_bytevec *vector;
	struct navi_pair head, *ptr = &head;

	vector = navi_bytevec(obj);
	for (size_t i = 0; i < vector->size; i++) {
		ptr->cdr = navi_make_empty_pair();
		ptr = &ptr->cdr.p->data->pair;
		ptr->car = navi_make_char(vector->data[i]);
	}
	ptr->cdr = navi_make_nil();
	return head.cdr;
}

/* FIXME: assumes ASCII */
navi_t navi_string_to_bytevec(navi_t string)
{
	struct navi_vector *svec = navi_vector(string);
	navi_t obj = navi_make_bytevec(svec->size);
	struct navi_bytevec *bvec = navi_bytevec(obj);

	for (size_t i = 0; i < svec->size; i++)
		bvec->data[i] = navi_char(svec->data[i]);

	return obj;
}

/* FIXME: assumes ASCII */
navi_t navi_bytevec_to_string(navi_t bytevec)
{
	struct navi_bytevec *bvec = navi_bytevec(bytevec);
	navi_t obj = navi_make_string(bvec->size, bvec->size, bvec->size);
	struct navi_vector *svec = navi_vector(obj);

	for (size_t i = 0; i < bvec->size; i++)
		svec->data[i] = navi_make_char(bvec->data[i]);

	return obj;
}

char *navi_bytevec_to_cstr(navi_t obj)
{
	struct navi_bytevec *vec = navi_bytevec(obj);
	char *cstr = navi_critical_malloc(vec->size + 1);

	for (size_t i = 0; i < vec->size; i++)
		cstr[i] = vec->data[i];
	cstr[vec->size] = '\0';
	return cstr;
}

DEFUN(scm_bytevectorp, args, env)
{
	return navi_make_bool(navi_type(navi_car(args)) == NAVI_BYTEVEC);
}

DEFUN(scm_make_bytevector, args, env)
{
	navi_t obj;
	int nr_args = navi_list_length(args);

	if (nr_args != 1 && nr_args != 2)
		navi_arity_error(env, navi_make_symbol("bytevector"));

	navi_type_check(navi_car(args), NAVI_NUM, env);

	obj = navi_make_bytevec(navi_num(navi_car(args)));
	if (nr_args == 2) {
		navi_type_check_byte(navi_cadr(args), env);
		bytevec_fill(obj, navi_num(navi_cadr(args)));
	}
	return obj;
}

DEFUN(scm_bytevector, args, env)
{
	return navi_list_to_bytevec(args, env);
}

DEFUN(scm_bytevector_length, args, env)
{
	navi_type_check(navi_car(args), NAVI_BYTEVEC, env);
	return navi_make_num(navi_bytevec(navi_car(args))->size);
}

DEFUN(scm_bytevector_u8_ref, args, env)
{
	navi_type_check(navi_car(args),  NAVI_BYTEVEC, env);
	navi_type_check_range(navi_cadr(args), 0, navi_bytevec(navi_car(args))->size, env);

	return navi_bytevec_ref(navi_car(args), navi_num(navi_cadr(args)));
}

DEFUN(scm_bytevector_u8_set, args, env)
{
	navi_type_check(navi_car(args), NAVI_BYTEVEC, env);
	navi_type_check_range(navi_cadr(args), 0, navi_bytevec(navi_car(args))->size, env);
	navi_type_check_byte(navi_caddr(args), env);

	navi_bytevec(navi_car(args))->data[navi_num(navi_cadr(args))] = navi_num(navi_caddr(args));
	return navi_unspecified();
}

DEFUN(scm_bytevector_append, args, env)
{
	navi_t cons, obj;
	struct navi_bytevec *vec;
	size_t i = 0, size = 0;

	navi_list_for_each(cons, args) {
		size += navi_bytevec_cast(navi_car(cons), NAVI_BYTEVEC, env)->size;
	}

	obj = navi_make_bytevec(size);
	vec = navi_bytevec(obj);

	navi_list_for_each(cons, args) {
		struct navi_bytevec *other = navi_bytevec(navi_car(cons));
		for (size_t j = 0; j < other->size; j++)
			vec->data[i++] = other->data[j];
	}
	vec->data[i] = '\0';
	return obj;
}

static navi_t copy_to(navi_t to, size_t at, navi_t from, size_t start,
		size_t end)
{
	struct navi_bytevec *tov = navi_bytevec(to), *fromv = navi_bytevec(from);
	for (size_t i = start; i < end; i++)
		tov->data[at++] = fromv->data[i];
	return to;
}

DEFUN(scm_bytevector_copy, args, env)
{
	navi_t from;
	long start, end;
	struct navi_bytevec *vec;
	int nr_args = navi_list_length(args);

	from = navi_type_check(navi_car(args), NAVI_BYTEVEC, env);
	vec = navi_bytevec(from);
	start = (nr_args > 1) ? navi_fixnum_cast(navi_cadr(args), env) : 0;
	end = (nr_args > 2) ? navi_fixnum_cast(navi_caddr(args), env) : (long) vec->size;

	navi_check_copy(vec->size, start, end, env);

	return copy_to(navi_make_bytevec(end - start), 0, from, start, end);
}

DEFUN(scm_bytevector_copy_to, args, env)
{
	navi_t to, from;
	long at, start, end;
	struct navi_bytevec *from_vec, *to_vec;
	int nr_args = navi_list_length(args);

	to = navi_type_check(navi_car(args), NAVI_BYTEVEC, env);
	at = navi_fixnum_cast(navi_cadr(args), env);
	from = navi_type_check(navi_caddr(args), NAVI_BYTEVEC, env);
	to_vec = navi_bytevec(to);
	from_vec = navi_bytevec(from);
	start = (nr_args > 3) ? navi_fixnum_cast(navi_cadddr(args), env) : 0;
	end = (nr_args > 4) ? navi_fixnum_cast(navi_caddddr(args), env)
		: (long)from_vec->size;

	navi_check_copy_to(to_vec->size, at, from_vec->size, start, end, env);

	copy_to(to, at, from, start, end);
	return navi_unspecified();
}

DEFUN(scm_utf8_to_string, args, env)
{
	navi_t r;
	long start, end;
	struct navi_bytevec *vec = navi_bytevec_cast(navi_car(args), NAVI_BYTEVEC, env);
	int nr_args = navi_list_length(args);

	start = (nr_args > 1) ? navi_fixnum_cast(navi_cadr(args), env) : 0;
	end = (nr_args > 2) ? navi_fixnum_cast(navi_caddr(args), env) : (long) vec->size;

	navi_check_copy(vec->size, start, end, env);
	if (!u_is_valid((char*)vec->data, start, end))
		navi_error(env, "invalid UTF-8");

	r = navi_make_string(end - start, end - start, end - start);
	memcpy(navi_string(r)->data, vec->data + start, end - start);
	navi_string(r)->length = u_strlen(navi_string(r)->data);

	return r;
}

DEFUN(scm_string_to_utf8, args, env)
{
	navi_t r;
	long start, end;
	struct navi_string *str = navi_string_cast(navi_car(args), env);
	int nr_args = navi_list_length(args);
	size_t size, i = 0;

	start = (nr_args > 1) ? navi_fixnum_cast(navi_cadr(args), env) : 0;
	end = (nr_args > 2) ? navi_fixnum_cast(navi_caddr(args), env)
		: (long) u_strlen((char*)str->data);

	navi_check_copy(str->length, start, end, env);

	u_skip_chars(str->data, start, &i);
	size = u_strsize(str->data + i, 0, end - start);
	r = navi_make_bytevec(size);
	memcpy(navi_bytevec(r)->data, str->data + i, size);

	return r;
}
