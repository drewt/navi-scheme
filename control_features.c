/* Copyright 2014-2015 Drew Thoreson
 *
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/.
 */

static _Noreturn void unhandled_exception(navi_obj obj, navi_env env)
{
	navi_obj p = navi_make_file_output_port(stderr);
	navi_port_write(navi_port(p), obj, env);
	fputc('\n', stderr);
	navi_close_output_port(navi_port(p), env);
	navi_die("no exception handler installed");
}

_Noreturn void navi_raise(navi_obj args, navi_env env)
{
	for (;;) {
		struct navi_procedure *proc;
		navi_obj expr = navi_env_lookup(env.dynamic, navi_sym_current_exn);
		if (navi_type(expr) != NAVI_PROCEDURE)
			unhandled_exception(args, env);

		/* set up environment and run handler */
		proc = navi_procedure(expr);
		navi_scope_unset(env.dynamic, navi_sym_current_exn);
		navi_force_tail(navi_apply(proc, args, env), env);
		/* handler returned: raise again */
	}
}

_Noreturn void _navi_error(navi_env env, navi_obj type, const char *msg, ...)
{
	va_list ap;
	navi_obj list;

	va_start(ap, msg);
	list = navi_make_pair(type, navi_vlist(navi_cstr_to_string(msg), ap));
	va_end(ap);

	navi_raise(navi_make_pair(list, navi_make_nil()), env);

}

DEFUN(procedurep, "procedure?", 1, 0, NAVI_ANY)
{
	return navi_make_bool(navi_type(scm_arg1) == NAVI_PROCEDURE);
}

DEFUN(apply, "apply", 2, NAVI_PROC_VARIADIC, NAVI_PROCEDURE, NAVI_ANY)
{
	navi_obj cons, last = scm_args;
	navi_list_for_each(cons, scm_args) {
		navi_obj fst = navi_car(cons);
		/* walk to the last argument */
		if (navi_type(navi_cdr(cons)) != NAVI_NIL) {
			last = cons;
			continue;
		}
		navi_type_check_proper_list(fst, scm_env);

		/* flatten arg list */
		navi_pair(last)->cdr = fst;
		break;
	}
	return navi_apply(navi_procedure(scm_arg1), navi_cdr(scm_args), scm_env);
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
	if (!navi_is_values(values))
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

	navi_obj result;
	struct navi_procedure *thunk = navi_procedure(scm_arg2);
	navi_env exn_env = navi_dynamic_env_new_scope(scm_env);
	navi_scope_set(exn_env.dynamic, navi_sym_current_exn, scm_arg1);
	result = navi_apply(thunk, navi_make_nil(), exn_env);
	navi_env_unref(exn_env);
	return result;
}

DEFUN(raise, "raise", 1, 0, NAVI_ANY)
{
	navi_raise(scm_args, scm_env);
}

DEFUN(raise_continuable, "raise-continuable", 1, 0, NAVI_ANY)
{
	navi_obj handler, result;
	struct navi_procedure *proc;

	handler = navi_env_lookup(scm_env.dynamic, navi_sym_current_exn);
	if (!navi_is_procedure(handler))
		unhandled_exception(scm_args, scm_env);

	proc = navi_procedure(handler);
	navi_scope_unset(scm_env.dynamic, navi_sym_current_exn);
	result = navi_apply(proc, scm_args, scm_env);
	navi_scope_set(scm_env.dynamic, navi_sym_current_exn, handler);
	return result;
}

DEFUN(error, "error", 1, NAVI_PROC_VARIADIC, NAVI_STRING)
{
	navi_raise(navi_make_pair(scm_args, navi_make_nil()), scm_env);
}

static bool is_error_object(navi_obj object)
{
	return navi_type(object) == NAVI_PAIR
		&& navi_type(navi_car(object)) == NAVI_SYMBOL
		&& navi_type(navi_cdr(object)) == NAVI_PAIR
		&& navi_type(navi_cadr(object)) == NAVI_STRING;
}

static navi_obj error_object_type(navi_obj object)
{
	return navi_car(object);
}

static navi_obj error_object_message(navi_obj object)
{
	return navi_cadr(object);
}

static navi_obj error_object_irritants(navi_obj object)
{
	return navi_cddr(object);
}

static navi_obj navi_type_check_error(navi_obj object, navi_env env)
{
	if (unlikely(!is_error_object(object)))
		navi_error(env, "type error: not an error object");
	return object;
}

DEFUN(error_objectp, "error-object?", 1, 0, NAVI_ANY)
{
	return navi_make_bool(is_error_object(scm_arg1));
}

DEFUN(error_object_message, "error-object-message", 1, 0, NAVI_ANY)
{
	navi_type_check_error(scm_arg1, scm_env);
	return error_object_message(scm_arg1);
}

DEFUN(error_object_irritants, "error-object-irritants", 1, 0, NAVI_ANY)
{
	navi_type_check_error(scm_arg1, scm_env);
	return error_object_irritants(scm_arg1);
}

DEFUN(read_errorp, "read-error?", 1, 0, NAVI_ANY)
{
	if (!is_error_object(scm_arg1))
		return navi_make_bool(false);
	return navi_make_bool(error_object_type(scm_arg1).p == navi_sym_read_error.p);
}

DEFUN(promisep, "promise?", 1, 0, NAVI_ANY)
{
	return navi_make_bool(navi_type(scm_arg1) == NAVI_PROMISE);
}

DEFUN(make_promise, "make-promise", 1, 0, NAVI_ANY)
{
	return navi_make_promise(scm_arg1, scm_env);
}
