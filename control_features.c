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

#include <stdarg.h>
#include <setjmp.h>

#include "navi.h"

_Noreturn void _navi_error(navi_env_t env, const char *msg, ...)
{
	va_list ap;
	navi_t list;

	va_start(ap, msg);
	list = navi_vlist(navi_cstr_to_string(msg), ap);
	va_end(ap);

	scm_raise(navi_make_pair(list, navi_make_nil()), env);
}

_Noreturn void navi_arity_error(navi_env_t env, const char *name)
{
	char buf[64];
	snprintf(buf, 64, "wrong number of arguments to '%s'", name);
	navi_error(env, buf);
}

DEFUN(scm_procedurep, args, env)
{
	return navi_make_bool(navi_type(navi_car(args)) == NAVI_FUNCTION);
}

DEFUN(scm_apply, args, env)
{
	navi_t cons, last = (navi_t) args;

	navi_list_for_each(cons, args) {
		navi_t fst = navi_car(cons);
		/* walk to the last argument */
		if (navi_type(navi_cdr(cons)) != NAVI_NIL) {
			last = cons;
			continue;
		}

		navi_type_check_list(fst, env);

		/* flatten arg list */
		navi_pair(last)->cdr = fst;
		break;
	}
	return navi_eval(args, env);
}

navi_t navi_call_escape(navi_t escape, navi_t arg)
{
	struct navi_escape *esc = navi_escape(escape);
	esc->arg = arg;
	longjmp(esc->state, 1);
}

DEFUN(scm_call_ec, args, env)
{
	struct navi_escape *escape;
	navi_t cont, call, fun = navi_car(args);

	navi_type_check_fun(fun, 1, env);

	cont = navi_make_escape();
	escape = navi_escape(cont);

	if (setjmp(escape->state))
		return escape->arg;

	call = navi_make_pair(fun, navi_make_pair(cont, navi_make_nil()));
	return navi_eval(call, env);
}

DEFUN(scm_values, args, env)
{
	navi_t values = navi_list_to_vector(values);
	values.p->type = NAVI_VALUES;
	return values;
}

DEFUN(scm_call_with_values, args, env)
{
	navi_t values, call_args;
	navi_type_check_fun(navi_car(args), 0, env);
	navi_type_check(navi_cadr(args), NAVI_FUNCTION, env);

	values = navi_eval(navi_make_pair(navi_car(args), navi_make_nil()), env);
	if (navi_type(values) != NAVI_VALUES)
		call_args = navi_make_pair(values, navi_make_nil());
	else
		call_args = navi_vector_to_list(values);

	return navi_eval(navi_make_pair(navi_cadr(args), call_args), env);
}

DEFUN(scm_with_exception_handler, args, env)
{
	navi_type_check_fun(navi_car(args),  1, env);
	navi_type_check_fun(navi_cadr(args), 0, env);

	struct navi_function *thunk = navi_fun(navi_cadr(args));
	navi_env_t exn_env = navi_env_new_scope(thunk->env);
	navi_scope_set(exn_env, navi_sym_exn, navi_car(args));

	if (thunk->builtin)
		return thunk->fn(navi_make_nil(), exn_env);
	return scm_begin(thunk->body, exn_env);
}

DEFUN(scm_raise, args, env)
{
	navi_t expr;
	struct navi_function *fun;

	for (;;) {
		expr = navi_env_lookup(env, navi_sym_exn);
		if (navi_type(expr) != NAVI_FUNCTION)
			die("no exception handler installed");

		/* set up environment and run handler */
		fun = navi_fun(expr);
		navi_scope_unset(env, navi_sym_exn);
		if (fun->builtin) {
			fun->fn(args, env);
		} else {
			navi_scope_set(env, navi_car(fun->args), navi_car(args));
			navi_eval(navi_make_pair(navi_sym_begin, fun->body), env);
		}
		/* handler returned: raise again */
	}
}

DEFUN(scm_raise_continuable, args, env)
{
	navi_t handler, result;
	struct navi_function *fun;

	handler = navi_env_lookup(env, navi_sym_exn);
	if (navi_type(handler) != NAVI_FUNCTION)
		die("no exception handler installed");

	fun = navi_fun(handler);
	navi_scope_unset(env, navi_sym_exn);
	if (fun->builtin) {
		result = fun->fn(args, env);
	} else {
		navi_scope_set(env, navi_car(fun->args), navi_car(args));
		result = navi_eval(navi_make_pair(navi_sym_begin, fun->body), env);
	}
	navi_scope_set(env, navi_sym_exn, handler);
	return result;
}

DEFUN(scm_error, args, env)
{
	navi_type_check(navi_car(args), NAVI_STRING, env);
	scm_raise(navi_make_pair(args, navi_make_nil()), env);
}

static bool is_error_object(navi_t object)
{
	return navi_type(object) == NAVI_PAIR && navi_is_proper_list(object) &&
		navi_type(navi_car(object)) == NAVI_STRING;
}

static navi_t navi_type_check_error(navi_t object, navi_env_t env)
{
	if (!is_error_object(object))
		navi_error(env, "type error");
	return object;
}

DEFUN(scm_error_objectp, args, env)
{
	return navi_make_bool(is_error_object(navi_car(args)));
}

DEFUN(scm_error_object_message, args, env)
{
	navi_type_check_error(navi_car(args), env);
	return navi_caar(args);
}

DEFUN(scm_error_object_irritants, args, env)
{
	navi_type_check_error(navi_car(args), env);
	return navi_cdar(args);
}

DEFUN(scm_read_errorp, args, env)
{
	if (!navi_bool(scm_error_objectp(args, env)))
		return navi_make_bool(false);
	if (navi_list_length(navi_car(args)) < 2)
		return navi_make_bool(false);
	return navi_make_bool(navi_symbol_eq(navi_cadr(navi_car(args)), navi_sym_read_error));
}

DEFUN(scm_promisep, args, env)
{
	return navi_make_bool(navi_type(navi_car(args)) == NAVI_PROMISE);
}

DEFUN(scm_make_promise, args, env)
{
	return navi_make_promise(navi_car(args), env);
}
