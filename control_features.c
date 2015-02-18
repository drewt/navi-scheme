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

// FIXME: broken, need to do lookup in dynamic environment
_Noreturn void navi_raise(navi_obj args, navi_env env)
{
	for (;;) {
		struct navi_procedure *proc;
		navi_obj expr = navi_env_lookup(env, navi_sym_exn);
		if (navi_type(expr) != NAVI_PROCEDURE)
			navi_die("no exception handler installed");

		/* set up environment and run handler */
		proc = navi_procedure(expr);
		navi_scope_unset(env.lexical, navi_sym_exn);
		navi_force_tail(navi_apply(proc, args, env), env);
		/* handler returned: raise again */
	}
}

_Noreturn void _navi_error(navi_env env, const char *msg, ...)
{
	va_list ap;
	navi_obj list;

	va_start(ap, msg);
	list = navi_vlist(navi_cstr_to_string(msg), ap);
	va_end(ap);

	navi_raise(navi_make_pair(list, navi_make_nil()), env);

}

DEFUN(procedurep, "procedure?", 1, 0, NAVI_ANY)
{
	return navi_make_bool(navi_type(scm_arg1) == NAVI_PROCEDURE);
}

DEFUN(apply, "apply", 2, NAVI_PROC_VARIADIC, NAVI_PROCEDURE, NAVI_ANY)
{
	navi_obj cons, last = (navi_obj) scm_args;
	navi_list_for_each(cons, scm_args) {
		navi_obj fst = navi_car(cons);
		/* walk to the last argument */
		if (navi_type(navi_cdr(cons)) != NAVI_NIL) {
			last = cons;
			continue;
		}
		navi_type_check_list(fst, scm_env);

		/* flatten arg list */
		navi_pair(last)->cdr = fst;
		break;
	}
	return navi_eval(scm_args, scm_env);
}

navi_obj navi_call_escape(navi_obj escape, navi_obj arg, navi_env env)
{
	struct navi_escape *esc = navi_escape(escape);
	esc->arg = arg;

	// TODO: swap dynamic environment
	longjmp(esc->state, 1);
}

DEFUN(call_ec, "call/ec", 1, 0, NAVI_PROCEDURE)
{
	navi_obj cont;
	struct navi_escape *escape;
	struct navi_procedure *proc = navi_procedure(scm_arg1);

	navi_check_arity(scm_arg1, 1, scm_env);

	cont = navi_make_escape();
	escape = navi_escape(cont);
	escape->env = scm_env;

	if (setjmp(escape->state))
		return escape->arg;

	return navi_force_tail(navi_apply(proc, navi_make_pair(cont, navi_make_nil()), scm_env), scm_env);
}

DEFUN(values, "values", 1, NAVI_PROC_VARIADIC, NAVI_ANY)
{
	navi_obj values = navi_list_to_vector(scm_args);
	values.p->type = NAVI_VALUES;
	return values;
}

DEFUN(call_with_values, "call-with-values", 2, 0, NAVI_PROCEDURE, NAVI_PROCEDURE)
{
	navi_obj values, call_args;
	navi_check_arity(scm_arg1, 0, scm_env);

	values = navi_eval(navi_make_pair(scm_arg1, navi_make_nil()), scm_env);
	if (navi_type(values) != NAVI_VALUES)
		call_args = navi_make_pair(values, navi_make_nil());
	else
		call_args = navi_vector_to_list(values);

	return navi_eval(navi_make_pair(scm_arg2, call_args), scm_env);
}

DEFUN(with_exception_handler, "with-exception-handler", 2, 0,
		NAVI_PROCEDURE, NAVI_PROCEDURE)
{
	navi_check_arity(scm_arg1, 1, scm_env);
	navi_check_arity(scm_arg2, 0, scm_env);

	struct navi_procedure *thunk = navi_procedure(scm_arg2);
	navi_env exn_env = navi_env_new_scope(thunk->env);
	navi_scope_set(exn_env.lexical, navi_sym_exn, scm_arg1);

	// FIXME: completely broken, need dynamic environment
	return _navi_apply(thunk, navi_make_nil(), exn_env, scm_env);
}

DEFUN(raise, "raise", 1, 0, NAVI_ANY)
{
	navi_raise(scm_args, scm_env);
}

DEFUN(raise_continuable, "raise-continuable", 1, 0, NAVI_ANY)
{
	navi_obj handler, result;
	struct navi_procedure *proc;

	handler = navi_env_lookup(scm_env, navi_sym_exn);
	if (navi_type(handler) != NAVI_PROCEDURE)
		navi_die("no exception handler installed");

	proc = navi_procedure(handler);
	navi_scope_unset(scm_env.lexical, navi_sym_exn);
	result = navi_apply(proc, scm_args, scm_env);
	navi_scope_set(scm_env.lexical, navi_sym_exn, handler);
	return result;
}

DEFUN(error, "error", 1, NAVI_PROC_VARIADIC, NAVI_STRING)
{
	navi_raise(navi_make_pair(scm_args, navi_make_nil()), scm_env);
}

static bool is_error_object(navi_obj object)
{
	return navi_type(object) == NAVI_PAIR && navi_is_proper_list(object) &&
		navi_type(navi_car(object)) == NAVI_STRING;
}

static navi_obj navi_type_check_error(navi_obj object, navi_env env)
{
	if (!is_error_object(object))
		navi_error(env, "type error");
	return object;
}

DEFUN(error_objectp, "error-object?", 1, 0, NAVI_ANY)
{
	return navi_make_bool(is_error_object(scm_arg1));
}

DEFUN(error_object_message, "error-object-message", 1, 0, NAVI_ANY)
{
	navi_type_check_error(scm_arg1, scm_env);
	return navi_car(scm_arg1);
}

DEFUN(error_object_irritants, "error-object-irritants", 1, 0, NAVI_ANY)
{
	navi_type_check_error(scm_arg1, scm_env);
	return navi_cdr(scm_arg1);
}

DEFUN(read_errorp, "read-error?", 1, 0, NAVI_ANY)
{
	if (!is_error_object(scm_arg1))
		return navi_make_bool(false);
	if (navi_list_length(scm_arg1) < 2)
		return navi_make_bool(false);
	return navi_make_bool(navi_symbol_eq(navi_cadr(scm_arg1), navi_sym_read_error));
}

DEFUN(promisep, "promise?", 1, 0, NAVI_ANY)
{
	return navi_make_bool(navi_type(scm_arg1) == NAVI_PROMISE);
}

DEFUN(make_promise, "make-promise", 1, 0, NAVI_ANY)
{
	return navi_make_promise(scm_arg1, scm_env);
}
