/* Copyright 2014-2015 Drew Thoreson
 *
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/.
 */

/*
 * extern.c: wrappers for inline functions
 *
 * This file defines alternate versions of various inline functions with
 * external linkage.  This allows them to be used as inline functions in NAVI
 * source, and regular functions in linked code.
 */

void _navi_extern_scope_unref(struct navi_scope *scope)
{
	_navi_scope_unref(scope);
}

struct navi_scope *_navi_extern_scope_ref(struct navi_scope *scope)
{
	return _navi_scope_ref(scope);
}

void navi_extern_env_ref(navi_env env)
{
	navi_env_ref(env);
}

struct navi_pair *navi_extern_pair(navi_obj obj)
{
	return navi_pair(obj);
}

struct navi_port *navi_extern_port(navi_obj obj)
{
	return navi_port(obj);
}

struct navi_string *navi_extern_string(navi_obj obj)
{
	return navi_string(obj);
}

struct navi_symbol *navi_extern_symbol(navi_obj obj)
{
	return navi_symbol(obj);
}

struct navi_vector *navi_extern_vector(navi_obj obj)
{
	return navi_vector(obj);
}

struct navi_bytevec *navi_extern_bytevec(navi_obj obj)
{
	return navi_bytevec(obj);
}

struct navi_procedure *navi_extern_procedure(navi_obj obj)
{
	return navi_procedure(obj);
}

struct navi_thunk *navi_extern_thunk(navi_obj obj)
{
	return navi_thunk(obj);
}

struct navi_escape *navi_extern_escape(navi_obj obj)
{
	return navi_escape(obj);
}

navi_env navi_extern_environment(navi_obj obj)
{
	return navi_environment(obj);
}

navi_obj navi_extern_car(navi_obj obj)
{
	return navi_car(obj);
}

navi_obj navi_extern_cdr(navi_obj obj)
{
	return navi_cdr(obj);
}

navi_obj navi_extern_make_void(void)
{
	return navi_make_void();
}

navi_obj navi_extern_make_nil(void)
{
	return navi_make_nil();
}

navi_obj navi_extern_make_eof(void)
{
	return navi_make_eof();
}

navi_obj navi_extern_make_fixnum(long num)
{
	return navi_make_fixnum(num);
}

navi_obj navi_extern_make_bool(int b)
{
	return navi_make_bool(b);
}

navi_obj navi_extern_make_char(unsigned long c)
{
	return navi_make_char(c);
}

navi_obj navi_extern_env_lookup(struct navi_scope *scope, navi_obj symbol)
{
	return navi_env_lookup(scope, symbol);
}

navi_obj navi_extern_apply(struct navi_procedure *proc, navi_obj args,
		navi_env env)
{
	return navi_apply(proc, args, env);
}

enum navi_type navi_extern_type(navi_obj obj)
{
	return navi_type(obj);
}

const char *navi_extern_strtype(enum navi_type type)
{
	return navi_strtype(type);
}

int navi_extern_is_void(navi_obj obj)
{
	return navi_is_void(obj);
}

int navi_extern_is_nil(navi_obj obj)
{
	return navi_is_nil(obj);
}

int navi_extern_is_eof(navi_obj obj)
{
	return navi_is_eof(obj);
}

int navi_extern_is_fixnum(navi_obj obj)
{
	return navi_is_fixnum(obj);
}

int navi_extern_is_bool(navi_obj obj)
{
	return navi_is_bool(obj);
}

int navi_extern_is_char(navi_obj obj)
{
	return navi_is_char(obj);
}

int navi_extern_is_values(navi_obj obj)
{
	return navi_is_values(obj);
}

int navi_extern_is_pair(navi_obj obj)
{
	return navi_is_pair(obj);
}

int navi_extern_is_port(navi_obj obj)
{
	return navi_is_port(obj);
}

int navi_extern_is_string(navi_obj obj)
{
	return navi_is_string(obj);
}

int navi_extern_is_symbol(navi_obj obj)
{
	return navi_is_symbol(obj);
}

int navi_extern_is_vector(navi_obj obj)
{
	return navi_is_vector(obj);
}

int navi_extern_is_bytevec(navi_obj obj)
{
	return navi_is_bytevec(obj);
}

int navi_extern_is_macro(navi_obj obj)
{
	return navi_is_macro(obj);
}

int navi_extern_is_procedure(navi_obj obj)
{
	return navi_is_procedure(obj);
}

int navi_extern_is_caselambda(navi_obj obj)
{
	return navi_is_caselambda(obj);
}

int navi_extern_is_escape(navi_obj obj)
{
	return navi_is_escape(obj);
}

int navi_extern_is_parameter(navi_obj obj)
{
	return navi_is_parameter(obj);
}

int navi_extern_is_environment(navi_obj obj)
{
	return navi_is_environment(obj);
}

int navi_extern_is_byte(navi_obj obj)
{
	return navi_is_byte(obj);
}

int navi_extern_is_list(navi_obj obj)
{
	return navi_is_list(obj);
}

void navi_extern_set_car(navi_obj cons, navi_obj obj)
{
	navi_set_car(cons, obj);
}

void navi_extern_set_cdr(navi_obj cons, navi_obj obj)
{
	navi_set_cdr(cons, obj);
}

void navi_extern_display(navi_obj obj, navi_env env)
{
	navi_display(obj, env);
}

void navi_extern_write(navi_obj obj, navi_env env)
{
	navi_write(obj, env);
}

int navi_extern_port_is_input_port(struct navi_port *p)
{
	return navi_port_is_input_port(p);
}

int navi_extern_port_is_ouput_port(struct navi_port *p)
{
	return navi_port_is_output_port(p);
}

int navi_extern_is_input_port(navi_obj obj)
{
	return navi_is_input_port(obj);
}

int navi_extern_is_output_port(navi_obj obj)
{
	return navi_is_output_port(obj);
}

navi_obj navi_extern_vector_ref(navi_obj vec, size_t i)
{
	return navi_vector_ref(vec, i);
}

size_t navi_extern_vector_length(navi_obj vec)
{
	return navi_vector_length(vec);
}

navi_obj navi_extern_bytevec_ref(navi_obj vec, size_t i)
{
	return navi_bytevec_ref(vec, i);
}

size_t navi_extern_bytevec_length(navi_obj vec)
{
	return navi_bytevec_length(vec);
}

int navi_extern_is_true(navi_obj obj)
{
	return navi_is_true(obj);
}

navi_obj navi_extern_force_tail(navi_obj obj, navi_env env)
{
	return navi_force_tail(obj, env);
}
