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

#include <stdio.h>
#include <stddef.h>
#include <string.h>
#include "sexp.h"

static _Noreturn void unbound_identifier(sexp_t ident, env_t env)
{
	error(env, "unbound identifier", make_apair("identifier", ident));
}

static bool list_of(sexp_t list, unsigned type, bool allow_dotted_tail)
{
	sexp_t cons;

	sexp_list_for_each(cons, list) {
		if (sexp_type(car(cons)) != type)
			return false;
	}
	if (!allow_dotted_tail && sexp_type(cons) != SEXP_NIL)
		return false;
	if (allow_dotted_tail && sexp_type(cons) != type)
		return false;
	return true;
}

static inline bool can_bounce(sexp_t sexp)
{
	enum sexp_type type = sexp_type(sexp);
	return (type == SEXP_SYMBOL || type == SEXP_PAIR);
}

static sexp_t eval_tail(sexp_t tail, env_t env)
{
	if (!can_bounce(tail))
		return tail;

	return make_bounce(tail, capture_env(env));
}

static bool lambda_valid(sexp_t lambda)
{
	if (sexp_type(lambda) != SEXP_PAIR)
		return false;
	if (sexp_type(cdr(lambda)) != SEXP_PAIR)
		return false;
	if (sexp_type(car(lambda)) == SEXP_SYMBOL)
		return true;
	if (!list_of(car(lambda), SEXP_SYMBOL, true))
		return false;
	return true;
}

DEFSPECIAL(eval_lambda, lambda, env)
{
	if (!lambda_valid(lambda))
		error(env, "invalid lambda list");

	return make_function(car(lambda), cdr(lambda), "?", env);
}

DEFSPECIAL(eval_caselambda, caselambda, env)
{
	sexp_t cons, sexp;
	struct sexp_vector *vec;
	unsigned i = 0;

	sexp = make_caselambda(list_length(caselambda));
	vec = sexp_vector(sexp);

	sexp_list_for_each(cons, caselambda) {
		if (!lambda_valid(car(cons)))
			error(env, "invalid case-lambda list");
		vec->data[i++] = make_function(caar(cons), cdar(cons), "?", env);
	}

	return sexp;
}

static sexp_t eval_defvar(sexp_t sym, sexp_t rest, env_t env)
{
	if (sexp_type(cdr(rest)) != SEXP_NIL)
		error(env, "too many arguments to define");

	scope_set(env, sym, trampoline(car(rest), env));
	return unspecified();
}

static sexp_t eval_defun(sexp_t fundecl, sexp_t rest, env_t env)
{
	sexp_t fun, name;

	if (!list_of(fundecl, SEXP_SYMBOL, true))
		error(env, "invalid defun list");

	name = car(fundecl);
	fun = make_function(cdr(fundecl), rest, bytevec_to_c_string(name), env);
	scope_set(env, name, fun);
	return unspecified();
}

DEFSPECIAL(eval_define, define, env)
{
	enum sexp_type type;

	if (list_length(define) < 2)
		error(env, "invalid define list");

	type = sexp_type(car(define));
	if (type == SEXP_SYMBOL)
		return eval_defvar(car(define), cdr(define), env);
	if (type == SEXP_PAIR)
		return eval_defun(car(define), cdr(define), env);
	error(env, "invalid define list");
}

static void extend_with_values(sexp_t vars, sexp_t vals, env_t env)
{
	sexp_t cons;
	size_t i = 0;

	if (sexp_type(vals) != SEXP_VALUES) {
		if (list_length(vars) != 1)
			error(env, "wrong number of values");
		scope_set(env, car(vars), vals);
		return;
	}

	if ((size_t)list_length(vars) != vector_length(vals))
		error(env, "wrong number of values");

	sexp_list_for_each(cons, vars) {
		scope_set(env, car(cons), vector_ref(vals, i++));
	}
}

DEFSPECIAL(eval_define_values, defvals, env)
{
	if (list_length(defvals) != 2)
		error(env, "invalid define-values list");

	extend_with_values(car(defvals), trampoline(cadr(defvals), env), env);
	return unspecified();
}

DEFSPECIAL(eval_defmacro, defmacro, env)
{
	sexp_t macro, name;

	if (list_length(defmacro) < 2 || !list_of(car(defmacro), SEXP_SYMBOL, true))
		error(env, "invalid define-macro list");

	name = caar(defmacro);
	macro = make_macro(cdar(defmacro), cdr(defmacro),
			bytevec_to_c_string(name), env);
	scope_set(env, name, macro);
	return unspecified();
}

DEFSPECIAL(eval_begin, begin, env)
{
	sexp_t cons, result;

	sexp_list_for_each(cons, begin) {
		if (last_pair(cons))
			return eval_tail(car(cons), env);
		result = trampoline(car(cons), env);
	}
	return result;
}

static bool let_def_valid(sexp_t def)
{
	return sexp_type(def) == SEXP_PAIR &&
		sexp_type(cdr(def)) == SEXP_PAIR &&
		sexp_type(cddr(def)) == SEXP_NIL &&
		sexp_type(car(def)) == SEXP_SYMBOL;
}

static bool let_defs_valid(sexp_t list)
{
	sexp_t cons;

	sexp_list_for_each(cons, list) {
		if (!let_def_valid(car(cons)))
			return false;
	}
	return sexp_type(cons) == SEXP_NIL;
}

static bool let_values_def_valid(sexp_t def)
{
	return sexp_type(def) == SEXP_PAIR &&
		sexp_type(cdr(def)) == SEXP_PAIR &&
		sexp_type(cddr(def)) == SEXP_NIL &&
		list_of(car(def), SEXP_SYMBOL, false);
}

static bool letvals_defs_valid(sexp_t list)
{
	sexp_t cons;

	sexp_list_for_each(cons, list) {
		if (!let_values_def_valid(car(cons)))
			return false;
	}
	return sexp_type(cons) == SEXP_NIL;
}

static env_t let_extend_env(sexp_t def_list, env_t env)
{
	sexp_t cons;
	env_t new = env_new_scope(env);

	sexp_list_for_each(cons, def_list) {
		sexp_t defn = car(cons);
		sexp_t val = trampoline(cadr(defn), env);
		scope_set(new, car(defn), val);
	}

	return new;
}

static env_t sequential_let_extend_env(sexp_t def_list, env_t env)
{
	sexp_t cons;
	env_t new = env_new_scope(env);

	sexp_list_for_each(cons, def_list) {
		sexp_t defn = car(cons);
		sexp_t val = trampoline(cadr(defn), new);
		scope_set(new, car(defn), val);
	}

	return new;
}

static env_t letvals_extend_env(sexp_t def_list, env_t env)
{
	sexp_t cons;
	env_t new = env_new_scope(env);

	sexp_list_for_each(cons, def_list) {
		sexp_t vals = trampoline(cadar(cons), env);
		extend_with_values(caar(cons), vals, env);
	}

	return new;
}

#define DEFLET(name, strname, validate, extend) \
	DEFSPECIAL(name, let, env) \
	{ \
		sexp_t result; \
		env_t new_env; \
		\
		if (!validate(car(let))) \
			error(env, "invalid " strname " list"); \
		\
		new_env = extend(car(let), env); \
		result = eval_begin(cdr(let), new_env); \
		scope_unref(new_env); \
		return result; \
	}

DEFLET(eval_let, "let", let_defs_valid, let_extend_env)
DEFLET(eval_sequential_let, "let*", let_defs_valid, sequential_let_extend_env)
DEFLET(eval_let_values, "let-values", letvals_defs_valid, letvals_extend_env)

static inline bool set_valid(sexp_t set)
{
	return sexp_type(car(set)) == SEXP_SYMBOL &&
		sexp_type(cdr(set)) == SEXP_PAIR &&
		sexp_type(cddr(set)) == SEXP_NIL;
}

DEFSPECIAL(eval_set, set, env)
{
	struct sexp_binding *binding;
	sexp_t value;

	if (!set_valid(set))
		error(env, "invalid set list");

	binding = env_binding(env, car(set));
	if (binding == NULL)
		unbound_identifier(car(set), env);

	value = trampoline(cadr(set), env);
	binding->object = value;

	return unspecified();
}

DEFSPECIAL(eval_quote, quote, env)
{
	if (sexp_type(cdr(quote)) != SEXP_NIL)
		error(env, "invalid argument to quote");
	return car(quote);
}

DEFSPECIAL(eval_unquote, unquote, env)
{
	if (list_length(unquote) != 1)
		error(env, "wrong number of arguments to unquote");
	return trampoline(car(unquote), env);
}

/* FIXME: this is incredibly ugly */
static sexp_t eval_qq(sexp_t sexp, env_t env)
{
	struct sexp_pair head, *last;
	sexp_t cons;

	switch (sexp_type(sexp)) {
	case SEXP_PAIR:
		if (symbol_eq(car(sexp), sym_unquote))
			return eval_unquote(cdr(sexp), env);

		last = &head;
		sexp_list_for_each(cons, sexp) {
			sexp_t elm = car(cons);
			if (is_pair(elm) && symbol_eq(car(elm), sym_splice)) {
				sexp_t list = eval_unquote(cdar(cons), env);
				if (is_proper_list(list)) {
					last->cdr = list;
					last = sexp_pair(last_cons(list));
				} // TODO: otherwise... ???
			} else if (symbol_eq(elm, sym_unquote)) {
				/* unquote in dotted tail */
				break;
			} else {
				last->cdr = make_empty_pair();
				last = sexp_pair(last->cdr);
				last->car = eval_qq(car(cons), env);
			}
		}
		last->cdr = is_nil(cons) ? make_nil() : eval_qq(cons, env);
		return head.cdr;
	case SEXP_VECTOR:
		return list_to_vector(eval_qq(vector_to_list(sexp), env));
	default: break;
	}
	return sexp;
}

DEFSPECIAL(eval_quasiquote, quote, env)
{
	if (!is_nil(cdr(quote)))
		error(env, "too many arguments to quasiquote");
	return eval_qq(car(quote), env);	
}

static bool case_valid(sexp_t scase)
{
	sexp_t cons;

	if (list_length(scase) < 2)
		return false;

	sexp_list_for_each(cons, cdr(scase)) {
		sexp_t fst = car(cons);
		if (list_length(fst) < 2)
			return false;
		if (!symbol_eq(fst, sym_else) && !is_proper_list(fst))
			return false;
	}
	return true;
}

static sexp_t eval_clause(sexp_t arg, sexp_t begin, env_t env)
{
	if (symbol_eq(car(begin), sym_eq_lt)) {
		sexp_t fun  = trampoline(cadr(begin), env);
		sexp_t call = make_pair(fun, make_pair(arg, make_nil()));
		return eval(call, env);
	}
	return eval_begin(begin, env);
}

DEFSPECIAL(eval_case, scase, env)
{
	sexp_t test, cons, inner;

	if (!case_valid(scase))
		error(env, "invalid case list");

	test = trampoline(car(scase), env);
	sexp_list_for_each(cons, cdr(scase)) {
		sexp_t fst = car(cons);
		if (symbol_eq(car(fst), sym_else))
			return eval_clause(test, cdr(fst), env);
		sexp_list_for_each(inner, car(fst)) {
			if (eqvp(car(inner), test))
				return eval_clause(test, cdr(fst), env);
		}
	}
	return unspecified();
}

static bool cond_valid(sexp_t cond)
{
	sexp_t cons;

	sexp_list_for_each(cons, cond) {
		int length;
		sexp_t fst = car(cons);
		if (!is_proper_list(fst))
			return false;
		if ((length = list_length(fst)) < 2)
			return false;
		if (symbol_eq(cadr(fst), sym_eq_lt) && length != 3)
			return false;
	}
	return sexp_type(cons) == SEXP_NIL;
}

static sexp_t eval_cond_clause(sexp_t test, sexp_t clause, env_t env)
{
	if (symbol_eq(car(clause), sym_eq_lt)) {
		sexp_t fun = trampoline(cadr(clause), env);
		sexp_t call = make_pair(fun, make_pair(test, make_nil()));
		return eval(call, env);
	}
	return eval_begin(clause, env);
}

DEFSPECIAL(eval_cond, cond, env)
{
	sexp_t cons;

	if (!cond_valid(cond))
		error(env, "invalid cond list");

	sexp_list_for_each(cons, cond) {
		sexp_t test, fst = car(cond);

		if (symbol_eq(car(fst), sym_else))
			return eval_clause(make_bool(true), cdr(fst), env);

		test = trampoline(car(fst), env);
		if (sexp_is_true(test))
			return eval_cond_clause(test, cdr(fst), env);
	}
	return unspecified();
}

DEFSPECIAL(eval_if, sif, env)
{
	sexp_t test;
	int nr_args = list_length(sif);

	if (nr_args != 2 && nr_args != 3)
		error(env, "wrong number of arguments to 'if'");

	test = trampoline(car(sif), env);
	if (sexp_is_true(test))
		return eval_tail(cadr(sif), env);
	if (nr_args == 3)
		return eval_tail(caddr(sif), env);
	return unspecified();
}

DEFSPECIAL(eval_and, and, env)
{
	sexp_t cons;

	sexp_list_for_each(cons, and) {
		if (last_pair(cons))
			return eval_tail(car(cons), env);
		if (!sexp_is_true(trampoline(car(cons), env)))
			return make_bool(false);
	}
	return make_bool(true);
}

DEFSPECIAL(eval_or, or, env)
{
	sexp_t cons;

	sexp_list_for_each(cons, or) {
		if (last_pair(cons))
			return eval_tail(car(cons), env);
		if (sexp_is_true(trampoline(car(cons), env)))
			return make_bool(true);
	}
	return make_bool(false);
}

DEFSPECIAL(eval_delay, args, env)
{
	return make_promise(car(args), env);
}

DEFUN(scm_force, args)
{
	struct sexp_function *fun = sexp_fun(car(args));
	sexp_t r = trampoline(make_pair(sym_begin, fun->body), fun->env);
	fun->body = make_pair(sym_quote, make_pair(r, make_nil()));
	return r;
}

static sexp_t map_eval(sexp_t sexp, void *data)
{
	return trampoline(sexp, data);
}

static inline sexp_t make_args(sexp_t args, env_t env)
{
	if (sexp_type(args) == SEXP_NIL)
		return make_nil();
	return map(args, map_eval, env);
}

static sexp_t map_quote(sexp_t sexp, void *data)
{
	return make_pair(sym_quote, make_pair(sexp, make_nil()));
}

static inline sexp_t macro_call(sexp_t macro, sexp_t args, env_t env)
{
	sexp_t result;
	macro.p->type = SEXP_FUNCTION;
	result = trampoline(make_pair(macro, map(args, map_quote, NULL)), env);
	macro.p->type = SEXP_MACRO;
	return result;
}

static sexp_t caselambda_call(sexp_t lambda, sexp_t args, env_t env)
{
	int nr_args = list_length(args);
	struct sexp_vector *vec = sexp_vector(lambda);

	for (size_t i = 0; i < vec->size; i++) {
		struct sexp_function *fun = sexp_fun(vec->data[i]);
		if (fun->arity == nr_args || (fun->arity < nr_args && fun->variadic))
			return apply(fun, args, env);
	}
	error(env, "wrong number of arguments");
}

static sexp_t eval_call(sexp_t call, env_t env)
{
	int length;
	sexp_t sexp;
	sexp_t fun = trampoline(car(call), env);
	enum sexp_type type = sexp_type(fun);

	switch (type) {
	/* special: pass args unevaluated, return result */
	case SEXP_SPECIAL:
		return apply(sexp_fun(fun), cdr(call), env);
	/* function: pass args evaluated, return result */
	case SEXP_FUNCTION:
		sexp = make_args(cdr(call), env);
		return apply(sexp_fun(fun), sexp, env);
	/* macro: pass args unevaluated, return eval(result) */
	case SEXP_MACRO:
		sexp = macro_call(fun, cdr(call), env);
		return eval(sexp, env);
	/* escape: magic */
	case SEXP_ESCAPE:
		length = list_length(call);
		sexp = length < 2 ? make_nil() : cadr(call);
		return call_escape(fun, sexp);
	/* caselambda: magic */
	case SEXP_CASELAMBDA:
		return caselambda_call(fun, cdr(call), env);
	default: break;
	}
	error(env, "call of non-procedure", make_apair("value", fun));
}

sexp_t eval(sexp_t sexp, env_t env)
{
	sexp_t val;

	switch (sexp_type(sexp)) {
	case SEXP_VOID:
	case SEXP_NIL:
	case SEXP_EOF:
	case SEXP_NUM:
	case SEXP_BOOL:
	case SEXP_CHAR:
	case SEXP_PORT:
	case SEXP_STRING:
	case SEXP_VECTOR:
	case SEXP_BYTEVEC:
	case SEXP_MACRO:
	case SEXP_SPECIAL:
	case SEXP_FUNCTION:
	case SEXP_PROMISE:
	case SEXP_CASELAMBDA:
	case SEXP_ESCAPE:
	case SEXP_ENVIRONMENT:
	case SEXP_BOUNCE:
		return sexp;
	case SEXP_VALUES:
		return vector_ref(sexp, 0);
	case SEXP_SYMBOL:
		val = env_lookup(env, sexp);
		if (sexp_type(val) == SEXP_VOID) {
			unbound_identifier(sexp, env);
		}
		return val;
	case SEXP_PAIR:
		if (!is_proper_list(sexp))
			error(env, "malformed expression",
					make_apair("expression", sexp));

		return eval_call(sexp, env);
	}
	return unspecified();
}

sexp_t trampoline(sexp_t sexp, env_t env)
{
	sexp_t result;

	scope_ref(env);
	for (;;) {
		result = eval(sexp, env);
		if (sexp_type(result) != SEXP_BOUNCE)
			break;
		scope_unref(env); // unref for previous bounce
		sexp = bounce_object(result);
		env = bounce_env(result);
	}
	scope_unref(env);
	return result;
}

DEFUN(scm_eval, args)
{
	return trampoline(car(args), ____env);
}

static inline bool arity_satisfied(struct sexp_function *fun, sexp_t args)
{
	int nr_args = list_length(args);
	return (fun->variadic && nr_args  >= fun->arity)
		|| nr_args == fun->arity;
}

sexp_t apply(struct sexp_function *fun, sexp_t args, env_t env)
{
	env_t new_env;
	if (!arity_satisfied(fun, args))
		error(env, "wrong number of arguments");
	if (fun->builtin)
		return fun->fn(args, env);
	new_env = extend_environment(fun->env, fun->args, args);
	return eval_begin(fun->body, new_env);
}
