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

#include <stdarg.h>
#include <setjmp.h>

#include "sexp.h"

_Noreturn void _error(env_t env, const char *msg, ...)
{
	va_list ap;
	sexp_t list;

	va_start(ap, msg);
	list = vlist(to_string(msg), ap);
	va_end(ap);

	scm_raise(make_pair(list, make_nil()), env);
}

DEFUN(scm_procedurep, args)
{
	return make_bool(sexp_type(car(args)) == SEXP_FUNCTION);
}

DEFUN(scm_apply, args)
{
	sexp_t cons, last = (sexp_t) args;

	sexp_list_for_each(cons, args) {
		sexp_t fst = car(cons);
		/* walk to the last argument */
		if (sexp_type(cdr(cons)) != SEXP_NIL) {
			last = cons;
			continue;
		}

		type_check_list(fst);

		/* flatten arg list */
		sexp_pair(last)->cdr = fst;
		break;
	}
	return trampoline(args, ____env);
}

sexp_t call_escape(sexp_t escape, sexp_t arg)
{
	struct sexp_escape *esc = sexp_escape(escape);
	esc->arg = arg;
	longjmp(esc->state, 1);
}

DEFUN(scm_call_ec, args)
{
	struct sexp_escape *escape;
	sexp_t cont, call, fun = car(args);

	type_check_fun(fun, 1);

	cont = make_escape();
	escape = sexp_escape(cont);

	if (setjmp(escape->state))
		return escape->arg;

	call = make_pair(fun, make_pair(cont, make_nil()));
	return trampoline(call, ____env);
}

DEFUN(scm_values, args)
{
	return make_values(args);
}

DEFUN(scm_call_with_values, args)
{
	sexp_t values, call_args;
	type_check_fun(car(args), 0);
	type_check(cadr(args), SEXP_FUNCTION);

	values = trampoline(make_pair(car(args), make_nil()), ____env);
	if (sexp_type(values) != SEXP_VALUES)
		call_args = make_pair(values, make_nil());
	else
		call_args = vector_to_list(values);

	return eval(make_pair(cadr(args), call_args), ____env);
}

DEFUN(scm_with_exception_handler, args)
{
	type_check_fun(car(args),  1);
	type_check_fun(cadr(args), 0);

	struct sexp_function *thunk = sexp_fun(cadr(args));
	env_t exn_env = env_new_scope(thunk->env);
	scope_set(exn_env, sym_exn, car(args));

	if (thunk->builtin)
		return thunk->fn(make_nil(), exn_env);
	return eval_begin(thunk->body, exn_env);
}

DEFUN(scm_raise, args)
{
	sexp_t sexp;
	struct sexp_function *fun;

	for (;;) {
		sexp = env_lookup(____env, sym_exn);
		if (sexp_type(sexp) != SEXP_FUNCTION)
			die("no exception handler installed");

		/* set up environment and run handler */
		fun = sexp_fun(sexp);
		scope_unset(____env, sym_exn);
		if (fun->builtin) {
			fun->fn(args, ____env);
		} else {
			scope_set(____env, car(fun->args), car(args));
			trampoline(make_pair(sym_begin, fun->body), ____env);
		}
		/* handler returned: raise again */
	}
}

DEFUN(scm_raise_continuable, args)
{
	sexp_t handler, result;
	struct sexp_function *fun;

	handler = env_lookup(____env, sym_exn);
	if (sexp_type(handler) != SEXP_FUNCTION)
		die("no exception handler installed");

	fun = sexp_fun(handler);
	scope_unset(____env, sym_exn);
	if (fun->builtin) {
		result = fun->fn(args, ____env);
	} else {
		scope_set(____env, car(fun->args), car(args));
		result = trampoline(make_pair(sym_begin, fun->body), ____env);
	}
	scope_set(____env, sym_exn, handler);
	return result;
}

DEFUN(scm_error, args)
{
	type_check(car(args), SEXP_STRING);
	CALL(scm_raise, make_pair(args, make_nil()));
}

static bool is_error_object(sexp_t object)
{
	return sexp_type(object) == SEXP_PAIR && is_proper_list(object) &&
		sexp_type(car(object)) == SEXP_STRING;
}

#define type_check_error(obj) _type_check_error(obj, ____env)
static sexp_t _type_check_error(sexp_t object, env_t env)
{
	if (!is_error_object(object))
		error(env, "type error");
	return object;
}

DEFUN(scm_error_objectp, args)
{
	return make_bool(is_error_object(car(args)));
}

DEFUN(scm_error_object_message, args)
{
	type_check_error(car(args));
	return caar(args);
}

DEFUN(scm_error_object_irritants, args)
{
	type_check_error(car(args));
	return cdar(args);
}

DEFUN(scm_read_errorp, args)
{
	if (!sexp_bool(CALL(scm_error_objectp, args)))
		return make_bool(false);
	if (list_length(car(args)) < 2)
		return make_bool(false);
	return make_bool(symbol_eq(cadr(car(args)), sym_read_error));
}

struct map_apply_arg {
	sexp_t fun;
	env_t env;
};

static sexp_t map_apply(sexp_t elm, void *data)
{
	struct map_apply_arg *arg = data;
	return  trampoline(make_pair(arg->fun, make_pair(elm, make_nil())),
			arg->env);
}

DEFUN(scm_map, args)
{
	struct map_apply_arg arg;

	type_check_fun(car(args), 1);
	type_check_list(cadr(args));

	arg.fun = car(args);
	arg.env = ____env;

	return map(cadr(args), map_apply, &arg);
}

/* FIXME: This is unsafe for UTF-8 (or any other variable-width encoding).
 *
 *        Safe implementation:
 *
 *            (define (string-map f s)
 *              (vector->string
 *                (vector-map f (string->vector s))))
 */

static sexp_t string_map_ip(sexp_t fun, sexp_t sexp, env_t env)
{
	struct sexp_string *vec = _string_cast(sexp, env);

	for (size_t i = 0; i < vec->size; i++) {
		sexp_t call = list(fun, make_char(vec->data[i]), make_void());
		vec->data[i] = _char_cast(trampoline(call, env), env);
	}
	return sexp;
}

DEFUN(scm_string_map_ip, args)
{
	type_check_fun(car(args), 1);
	type_check(cadr(args), SEXP_STRING);

	return string_map_ip(car(args), cadr(args), ____env);
}

DEFUN(scm_string_map, args)
{
	type_check_fun(car(args), 1);
	type_check(cadr(args), SEXP_STRING);

	return string_map_ip(car(args), string_copy(cadr(args)), ____env);
}

static sexp_t vector_map(sexp_t fun, sexp_t to, sexp_t from, env_t env)
{
	struct sexp_vector *tov = _vector_cast(to, SEXP_VECTOR, env);
	struct sexp_vector *fromv = _vector_cast(from, SEXP_VECTOR, env);

	for (size_t i = 0; i < tov->size; i++) {
		sexp_t call = list(fun, fromv->data[i], make_void());
		tov->data[i] = trampoline(call, env);
	}
	return to;
}

DEFUN(scm_vector_map_ip, args)
{
	type_check_fun(car(args), 1);
	type_check(cadr(args), SEXP_VECTOR);
	return vector_map(car(args), cadr(args), cadr(args), ____env);
}

DEFUN(scm_vector_map, args)
{
	type_check_fun(car(args), 1);
	type_check(cadr(args), SEXP_VECTOR);
	return vector_map(car(args), make_vector(vector_length(cadr(args))),
			cadr(args), ____env);
}
