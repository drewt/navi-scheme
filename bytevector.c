/* Copyright 2014-2015 Drew Thoreson
 *
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/.
 */

#include "navi.h"
#include "navi/unicode.h"

bool navi_bytevec_equal(navi_obj obj, const char *cstr)
{
	struct navi_bytevec *vec = navi_bytevec(obj);
	for (size_t i = 0; i < vec->size; i++)
		if (vec->data[i] != (unsigned char) cstr[i])
			return false;
	return true;
}

static void bytevec_fill(navi_obj vector, unsigned char fill)
{
	struct navi_bytevec *vec = navi_bytevec(vector);

	for (size_t i = 0; i < vec->size; i++)
		vec->data[i] = fill;
}

navi_obj navi_list_to_bytevec(navi_obj list, navi_env env)
{
	navi_obj cons, vec = navi_make_bytevec(navi_list_length(list));

	unsigned i = 0;
	struct navi_bytevec *vector = navi_bytevec(vec);
	navi_list_for_each(cons, list) {
		navi_type_check_byte(navi_car(cons), env);
		vector->data[i++] = navi_fixnum(navi_car(cons));
	}
	return vec;
}

DEFUN(bytevectorp, "bytevector?", 1, 0, NAVI_ANY)
{
	return navi_make_bool(navi_type(scm_arg1) == NAVI_BYTEVEC);
}

DEFUN(make_bytevector, "make-bytevector", 1, NAVI_PROC_VARIADIC, NAVI_FIXNUM)
{
	navi_obj obj;

	if (scm_nr_args > 2)
		navi_arity_error(scm_env, navi_make_symbol("bytevector"));

	obj = navi_make_bytevec(navi_fixnum(scm_arg1));
	if (scm_nr_args == 2) {
		navi_type_check_byte(scm_arg2, scm_env);
		bytevec_fill(obj, navi_fixnum(scm_arg2));
	}
	return obj;
}

DEFUN(bytevector, "bytevector", 0, NAVI_PROC_VARIADIC)
{
	return navi_list_to_bytevec(scm_args, scm_env);
}

DEFUN(bytevector_length, "bytevector-length", 1, 0, NAVI_BYTEVEC)
{
	return navi_make_fixnum(navi_bytevec(scm_arg1)->size);
}

DEFUN(bytevector_u8_ref, "bytevector-u8-ref", 2, 0,
		NAVI_BYTEVEC, NAVI_FIXNUM)
{
	navi_type_check_range(scm_arg2, 0, navi_bytevec(scm_arg1)->size, scm_env);
	return navi_bytevec_ref(scm_arg1, navi_fixnum(scm_arg2));
}

DEFUN(bytevector_u8_set, "bytevector-u8-set!", 3, 0,
		NAVI_BYTEVEC, NAVI_FIXNUM, NAVI_BYTE)
{
	navi_type_check_range(scm_arg2, 0, navi_bytevec(scm_arg1)->size, scm_env);
	navi_bytevec(scm_arg1)->data[navi_fixnum(scm_arg2)] = navi_fixnum(scm_arg3);
	return navi_unspecified();
}

DEFUN(bytevector_append, "bytevector-append", 0, NAVI_PROC_VARIADIC)
{
	navi_obj cons, obj;
	struct navi_bytevec *vec;
	size_t i = 0, size = 0;

	navi_list_for_each(cons, scm_args) {
		size += navi_bytevec_cast(navi_car(cons), scm_env)->size;
	}

	obj = navi_make_bytevec(size);
	vec = navi_bytevec(obj);

	navi_list_for_each(cons, scm_args) {
		struct navi_bytevec *other = navi_bytevec(navi_car(cons));
		for (size_t j = 0; j < other->size; j++)
			vec->data[i++] = other->data[j];
	}
	vec->data[i] = '\0';
	return obj;
}

static navi_obj copy_to(navi_obj to, size_t at, navi_obj from, size_t start,
		size_t end)
{
	struct navi_bytevec *tov = navi_bytevec(to), *fromv = navi_bytevec(from);
	for (size_t i = start; i < end; i++)
		tov->data[at++] = fromv->data[i];
	return to;
}

DEFUN(bytevector_copy, "bytevector-copy", 1, NAVI_PROC_VARIADIC,
		NAVI_BYTEVEC)
{
	navi_obj from;
	long start, end;
	struct navi_bytevec *vec;

	from = scm_arg1;
	vec = navi_bytevec(from);
	start = (scm_nr_args > 1) ? navi_fixnum_cast(scm_arg2, scm_env) : 0;
	end = (scm_nr_args > 2) ? navi_fixnum_cast(scm_arg3, scm_env) : (long) vec->size;

	navi_check_copy(vec->size, start, end, scm_env);

	return copy_to(navi_make_bytevec(end - start), 0, from, start, end);
}

DEFUN(bytevector_copy_to, "bytevector-copy!", 3, NAVI_PROC_VARIADIC,
		NAVI_BYTEVEC, NAVI_FIXNUM, NAVI_BYTEVEC)
{
	navi_obj to, from;
	long at, start, end;
	struct navi_bytevec *from_vec, *to_vec;

	to = scm_arg1;
	at = navi_fixnum(scm_arg2);
	from = scm_arg3;
	to_vec = navi_bytevec(to);
	from_vec = navi_bytevec(from);
	start = (scm_nr_args > 3) ? navi_fixnum_cast(scm_arg4, scm_env) : 0;
	end = (scm_nr_args > 4) ? navi_fixnum_cast(scm_arg5, scm_env)
		: (long)from_vec->size;

	navi_check_copy_to(to_vec->size, at, from_vec->size, start, end, scm_env);

	copy_to(to, at, from, start, end);
	return navi_unspecified();
}

static int32_t count_chars(struct navi_bytevec *vec)
{
	int32_t n = 0;
	for (int32_t i = 0; (size_t) i < vec->size; n++) {
		UChar32 ch;
		u8_next_unchecked(vec->data, i, vec->size, ch);
		if (ch < 0)
			return -1;
	}
	return n;
}

DEFUN(utf8_to_string, "utf8->string", 1, NAVI_PROC_VARIADIC, NAVI_BYTEVEC)
{
	navi_obj str;
	int32_t start, end, size, length;
	struct navi_bytevec *vec = navi_bytevec(scm_arg1);

	start = (scm_nr_args > 1) ? navi_fixnum_cast(scm_arg2, scm_env) : 0;
	end = (scm_nr_args > 2) ? navi_fixnum_cast(scm_arg3, scm_env) : (long) vec->size;

	navi_check_copy(vec->size, start, end, scm_env);
	size = end - start;
	length = count_chars(vec);
	if (length < 0)
		navi_error(scm_env, "invalid UTF-8");

	str = navi_make_string(size, size, length);
	memcpy(navi_string(str)->data, vec->data + start, size);
	return str;
}

DEFUN(string_to_utf8, "string->utf8", 1, NAVI_PROC_VARIADIC, NAVI_STRING)
{
	navi_obj r;
	int32_t start, end;
	struct navi_string *str = navi_string(scm_arg1);
	int32_t end_i, start_i = 0;

	start = (scm_nr_args > 1) ? navi_fixnum_cast(scm_arg2, scm_env) : 0;
	end = (scm_nr_args > 2) ? navi_fixnum_cast(scm_arg3, scm_env) : str->length;

	navi_check_copy(str->length, start, end, scm_env);

	u8_fwd_n(str->data, start_i, str->size, start);
	end_i = start_i;
	for (int32_t count = start; count < end; count++) {
		u8_fwd_1(str->data, end_i, str->size);
	}
	r = navi_make_bytevec(end_i - start_i);
	memcpy(navi_bytevec(r)->data, str->data + start_i, end_i - start_i);

	return r;
}
