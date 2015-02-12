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

#include <stdio.h>
#include <stddef.h>
#include <string.h>
#include "navi.h"

static navi_t _eval(navi_t expr, navi_env_t env);

static inline navi_t bounce_object(navi_t bounce)
{
	return navi_car(bounce);
}

static inline navi_env_t bounce_env(navi_t bounce)
{
	return navi_cdr(bounce).p->data->env;
}

static _Noreturn void unbound_identifier(navi_t ident, navi_env_t env)
{
	navi_error(env, "unbound identifier", navi_make_apair("identifier", ident));
}

static bool list_of(navi_t list, unsigned type, bool allow_dotted_tail)
{
	navi_t cons;

	navi_list_for_each(cons, list) {
		if (navi_type(navi_car(cons)) != type)
			return false;
	}
	if (!allow_dotted_tail && navi_type(cons) != NAVI_NIL)
		return false;
	if (allow_dotted_tail && navi_type(cons) != NAVI_NIL
			&& navi_type(cons) != type)
		return false;
	return true;
}

static inline bool can_bounce(navi_t obj)
{
	enum navi_type type = navi_type(obj);
	return (type == NAVI_SYMBOL || type == NAVI_PAIR);
}

static navi_t eval_tail(navi_t tail, navi_env_t env)
{
	if (!can_bounce(tail))
		return tail;

	return navi_make_bounce(tail, navi_capture_env(env));
}

static bool lambda_valid(navi_t lambda)
{
	if (navi_type(lambda) != NAVI_PAIR)
		return false;
	if (navi_type(navi_cdr(lambda)) != NAVI_PAIR)
		return false;
	if (navi_type(navi_car(lambda)) == NAVI_SYMBOL)
		return true;
	if (!list_of(navi_car(lambda), NAVI_SYMBOL, true))
		return false;
	return true;
}

DEFSPECIAL(lambda, lambda, env, "lambda", 2, NAVI_PROC_VARIADIC,
		NAVI_LIST, NAVI_ANY)
{
	if (!lambda_valid(lambda))
		navi_error(env, "invalid lambda list");
	return navi_make_lambda(navi_car(lambda), navi_cdr(lambda), env);
}

DEFSPECIAL(caselambda, caselambda, env, "case-lambda", 1, NAVI_PROC_VARIADIC,
		NAVI_ANY)
{
	navi_t cons, expr;
	struct navi_vector *vec;
	unsigned i = 0;

	expr = navi_make_caselambda(navi_list_length(caselambda));
	vec = navi_vector(expr);

	navi_list_for_each(cons, caselambda) {
		if (!lambda_valid(navi_car(cons)))
			navi_error(env, "invalid case-lambda list");
		vec->data[i++] = navi_make_lambda(navi_caar(cons),
				navi_cdar(cons), env);
	}
	return expr;
}

static navi_t eval_defvar(navi_t sym, navi_t rest, navi_env_t env)
{
	if (navi_type(navi_cdr(rest)) != NAVI_NIL)
		navi_arity_error(env, navi_make_symbol("define"));

	navi_scope_set(env, sym, navi_eval(navi_car(rest), env));
	return navi_unspecified();
}

static navi_t eval_defun(navi_t fundecl, navi_t rest, navi_env_t env)
{
	navi_t proc, name;

	if (!list_of(fundecl, NAVI_SYMBOL, true))
		navi_error(env, "invalid defun list");

	name = navi_car(fundecl);
	proc = navi_make_procedure(navi_cdr(fundecl), rest, name, env);
	navi_scope_set(env, name, proc);
	return navi_unspecified();
}

DEFSPECIAL(define, define, env, "define", 2, NAVI_PROC_VARIADIC,
		NAVI_LIST, NAVI_ANY)
{
	enum navi_type type;

	if (navi_list_length(define) < 2)
		navi_error(env, "invalid define list");

	type = navi_type(navi_car(define));
	if (type == NAVI_SYMBOL)
		return eval_defvar(navi_car(define), navi_cdr(define), env);
	if (type == NAVI_PAIR)
		return eval_defun(navi_car(define), navi_cdr(define), env);
	navi_error(env, "invalid define list");
}

static void extend_with_values(navi_t vars, navi_t vals, navi_t which, navi_env_t env)
{
	navi_t cons;
	size_t i = 0;

	if (navi_type(vals) != NAVI_VALUES) {
		if (navi_list_length(vars) != 1)
			navi_arity_error(env, which);
		navi_scope_set(env, navi_car(vars), vals);
		return;
	}

	if ((size_t)navi_list_length(vars) != navi_vector_length(vals))
		navi_arity_error(env, which);

	navi_list_for_each(cons, vars) {
		navi_scope_set(env, navi_car(cons), navi_vector_ref(vals, i++));
	}
}

DEFSPECIAL(define_values, defvals, env, "define-values", 2, NAVI_PROC_VARIADIC,
		NAVI_LIST, NAVI_ANY)
{
	if (navi_list_length(defvals) != 2)
		navi_error(env, "invalid define-values list");

	extend_with_values(navi_car(defvals), navi_eval(navi_cadr(defvals), env),
			navi_make_symbol("define-values"), env);
	return navi_unspecified();
}

DEFSPECIAL(defmacro, defmacro, env, "defmacro", 2, NAVI_PROC_VARIADIC,
		NAVI_LIST, NAVI_ANY)
{
	navi_t macro, name;

	if (navi_list_length(defmacro) < 2 || !list_of(navi_car(defmacro), NAVI_SYMBOL, true))
		navi_error(env, "invalid define-macro list");

	name = navi_caar(defmacro);
	macro = navi_make_macro(navi_cdar(defmacro), navi_cdr(defmacro), name, env);
	navi_scope_set(env, name, macro);
	return navi_unspecified();
}

DEFSPECIAL(begin, begin, env, "begin", 1, NAVI_PROC_VARIADIC, NAVI_ANY)
{
	navi_t cons, result;

	navi_list_for_each(cons, begin) {
		if (navi_is_last_pair(cons))
			return eval_tail(navi_car(cons), env);
		result = navi_eval(navi_car(cons), env);
	}
	return result;
}

static bool let_def_valid(navi_t def)
{
	return navi_type(def) == NAVI_PAIR &&
		navi_type(navi_cdr(def)) == NAVI_PAIR &&
		navi_type(navi_cddr(def)) == NAVI_NIL &&
		navi_type(navi_car(def)) == NAVI_SYMBOL;
}

static bool let_defs_valid(navi_t list)
{
	navi_t cons;

	navi_list_for_each(cons, list) {
		if (!let_def_valid(navi_car(cons)))
			return false;
	}
	return navi_type(cons) == NAVI_NIL;
}

static bool let_values_def_valid(navi_t def)
{
	return navi_type(def) == NAVI_PAIR &&
		navi_type(navi_cdr(def)) == NAVI_PAIR &&
		navi_type(navi_cddr(def)) == NAVI_NIL &&
		list_of(navi_car(def), NAVI_SYMBOL, false);
}

static bool letvals_defs_valid(navi_t list)
{
	navi_t cons;

	navi_list_for_each(cons, list) {
		if (!let_values_def_valid(navi_car(cons)))
			return false;
	}
	return navi_type(cons) == NAVI_NIL;
}

static navi_env_t let_extend_env(navi_t def_list, navi_env_t env)
{
	navi_t cons;
	navi_env_t new = navi_env_new_scope(env);

	navi_list_for_each(cons, def_list) {
		navi_t defn = navi_car(cons);
		navi_t val = navi_eval(navi_cadr(defn), env);
		navi_scope_set(new, navi_car(defn), val);
	}

	return new;
}

static navi_env_t sequential_let_extend_env(navi_t def_list, navi_env_t env)
{
	navi_t cons;
	navi_env_t new = navi_env_new_scope(env);

	navi_list_for_each(cons, def_list) {
		navi_t defn = navi_car(cons);
		navi_t val = navi_eval(navi_cadr(defn), new);
		navi_scope_set(new, navi_car(defn), val);
	}

	return new;
}

static navi_env_t letvals_extend_env(navi_t def_list, navi_env_t env)
{
	navi_t cons;
	navi_env_t new = navi_env_new_scope(env);

	navi_list_for_each(cons, def_list) {
		navi_t vals = navi_eval(navi_cadar(cons), env);
		extend_with_values(navi_caar(cons), vals,
				navi_make_symbol("let-values"), env);
	}

	return new;
}

#define DEFLET(name, scmname, validate, extend) \
	DEFSPECIAL(name, let, env, scmname, 2, NAVI_PROC_VARIADIC, \
			NAVI_LIST, NAVI_ANY) \
	{ \
		navi_t result; \
		navi_env_t new_env; \
		\
		if (!validate(navi_car(let))) \
			navi_error(env, "invalid " scmname " list"); \
		\
		new_env = extend(navi_car(let), env); \
		result = scm_begin(navi_cdr(let), new_env); \
		navi_scope_unref(new_env); \
		return result; \
	}

DEFLET(let, "let", let_defs_valid, let_extend_env)
DEFLET(sequential_let, "let*", let_defs_valid, sequential_let_extend_env)
DEFLET(let_values, "let-values", letvals_defs_valid, letvals_extend_env)

static inline bool set_valid(navi_t set)
{
	return navi_type(navi_car(set)) == NAVI_SYMBOL &&
		navi_type(navi_cdr(set)) == NAVI_PAIR &&
		navi_type(navi_cddr(set)) == NAVI_NIL;
}

DEFSPECIAL(set, set, env, "set!", 2, 0, NAVI_ANY, NAVI_ANY)
{
	struct navi_binding *binding;
	navi_t value;

	if (!set_valid(set))
		navi_error(env, "invalid set list");

	binding = navi_env_binding(env, navi_car(set));
	if (binding == NULL)
		unbound_identifier(navi_car(set), env);

	value = navi_eval(navi_cadr(set), env);
	binding->object = value;

	return navi_unspecified();
}

DEFSPECIAL(quote, quote, env, "quote", 1, 0, NAVI_ANY)
{
	if (navi_type(navi_cdr(quote)) != NAVI_NIL)
		navi_error(env, "invalid argument to quote");
	return navi_car(quote);
}

DEFSPECIAL(unquote, unquote, env, "unquote", 1, 0, NAVI_ANY)
{
	if (navi_list_length(unquote) != 1)
		navi_arity_error(env, navi_make_symbol("unquote"));
	return navi_eval(navi_car(unquote), env);
}

/* FIXME: this is incredibly ugly */
static navi_t eval_qq(navi_t expr, navi_env_t env)
{
	struct navi_pair head, *last;
	navi_t cons;

	switch (navi_type(expr)) {
	case NAVI_PAIR:
		if (navi_symbol_eq(navi_car(expr), navi_sym_unquote))
			return scm_unquote(navi_cdr(expr), env);

		last = &head;
		navi_list_for_each(cons, expr) {
			navi_t elm = navi_car(cons);
			if (navi_is_pair(elm) && navi_symbol_eq(navi_car(elm), navi_sym_splice)) {
				navi_t list = scm_unquote(navi_cdar(cons), env);
				if (navi_is_proper_list(list)) {
					last->cdr = list;
					last = navi_pair(navi_last_cons(list));
				} // TODO: otherwise... ???
			} else if (navi_symbol_eq(elm, navi_sym_unquote)) {
				/* unquote in dotted tail */
				break;
			} else {
				last->cdr = navi_make_empty_pair();
				last = navi_pair(last->cdr);
				last->car = eval_qq(navi_car(cons), env);
			}
		}
		last->cdr = navi_is_nil(cons) ? navi_make_nil() : eval_qq(cons, env);
		return head.cdr;
	case NAVI_VECTOR:
		return navi_list_to_vector(eval_qq(navi_vector_to_list(expr), env));
	default: break;
	}
	return expr;
}

DEFSPECIAL(quasiquote, quote, env, "quasiquote", 1, 0, NAVI_ANY)
{
	if (!navi_is_nil(navi_cdr(quote)))
		navi_arity_error(env, navi_make_symbol("quasiquote"));
	return eval_qq(navi_car(quote), env);	
}

static bool case_valid(navi_t scase)
{
	navi_t cons;

	if (navi_list_length(scase) < 2)
		return false;

	navi_list_for_each(cons, navi_cdr(scase)) {
		navi_t fst = navi_car(cons);
		if (navi_list_length(fst) < 2)
			return false;
		if (!navi_symbol_eq(fst, navi_sym_else) && !navi_is_proper_list(fst))
			return false;
	}
	return true;
}

static navi_t eval_clause(navi_t arg, navi_t begin, navi_env_t env)
{
	if (navi_symbol_eq(navi_car(begin), navi_sym_eq_lt)) {
		navi_t proc = navi_eval(navi_cadr(begin), env);
		navi_t call = navi_make_pair(proc, navi_make_pair(arg, navi_make_nil()));
		return _eval(call, env);
	}
	return scm_begin(begin, env);
}

DEFSPECIAL(case, scase, env, "case", 2, NAVI_PROC_VARIADIC, NAVI_ANY, NAVI_ANY)
{
	navi_t test, cons, inner;

	if (!case_valid(scase))
		navi_error(env, "invalid case list");

	test = navi_eval(navi_car(scase), env);
	navi_list_for_each(cons, navi_cdr(scase)) {
		navi_t fst = navi_car(cons);
		if (navi_symbol_eq(navi_car(fst), navi_sym_else))
			return eval_clause(test, navi_cdr(fst), env);
		navi_list_for_each(inner, navi_car(fst)) {
			if (navi_eqvp(navi_car(inner), test))
				return eval_clause(test, navi_cdr(fst), env);
		}
	}
	return navi_unspecified();
}

static bool cond_valid(navi_t cond)
{
	navi_t cons;

	navi_list_for_each(cons, cond) {
		int length;
		navi_t fst = navi_car(cons);
		if (!navi_is_proper_list(fst))
			return false;
		if ((length = navi_list_length(fst)) < 2)
			return false;
		if (navi_symbol_eq(navi_cadr(fst), navi_sym_eq_lt) && length != 3)
			return false;
	}
	return navi_type(cons) == NAVI_NIL;
}

static navi_t scm_cond_clause(navi_t test, navi_t clause, navi_env_t env)
{
	if (navi_symbol_eq(navi_car(clause), navi_sym_eq_lt)) {
		navi_t proc = navi_eval(navi_cadr(clause), env);
		navi_t call = navi_make_pair(proc, navi_make_pair(test, navi_make_nil()));
		return _eval(call, env);
	}
	return scm_begin(clause, env);
}

DEFSPECIAL(cond, cond, env, "cond", 1, NAVI_PROC_VARIADIC, NAVI_ANY)
{
	navi_t cons;

	if (!cond_valid(cond))
		navi_error(env, "invalid cond list");

	navi_list_for_each(cons, cond) {
		navi_t test, fst = navi_car(cond);

		if (navi_symbol_eq(navi_car(fst), navi_sym_else))
			return eval_clause(navi_make_bool(true), navi_cdr(fst), env);

		test = navi_eval(navi_car(fst), env);
		if (navi_is_true(test))
			return scm_cond_clause(test, navi_cdr(fst), env);
	}
	return navi_unspecified();
}

DEFSPECIAL(if, sif, env, "if", 2, NAVI_PROC_VARIADIC, NAVI_ANY, NAVI_ANY)
{
	navi_t test;
	int nr_args = navi_list_length(sif);

	if (nr_args != 2 && nr_args != 3)
		navi_arity_error(env, navi_make_symbol("if"));

	test = navi_eval(navi_car(sif), env);
	if (navi_is_true(test))
		return eval_tail(navi_cadr(sif), env);
	if (nr_args == 3)
		return eval_tail(navi_caddr(sif), env);
	return navi_unspecified();
}

DEFSPECIAL(and, and, env, "and", 0, NAVI_PROC_VARIADIC)
{
	navi_t cons;

	navi_list_for_each(cons, and) {
		if (navi_is_last_pair(cons))
			return eval_tail(navi_car(cons), env);
		if (!navi_is_true(navi_eval(navi_car(cons), env)))
			return navi_make_bool(false);
	}
	return navi_make_bool(true);
}

DEFSPECIAL(or, or, env, "or", 0, NAVI_PROC_VARIADIC)
{
	navi_t cons;

	navi_list_for_each(cons, or) {
		if (navi_is_last_pair(cons))
			return eval_tail(navi_car(cons), env);
		if (navi_is_true(navi_eval(navi_car(cons), env)))
			return navi_make_bool(true);
	}
	return navi_make_bool(false);
}

DEFSPECIAL(delay, args, env, "delay", 1, 0, NAVI_ANY)
{
	return navi_make_promise(navi_car(args), env);
}

DEFUN(force, args, env, "force", 1, 0, NAVI_PROCEDURE)
{
	// FIXME: type checking...
	struct navi_procedure *proc = navi_procedure(navi_car(args));
	navi_t r = navi_eval(navi_make_pair(navi_sym_begin,
				proc->body), proc->env);
	proc->body = navi_make_pair(navi_sym_quote,
			navi_make_pair(r, navi_make_nil()));
	return r;
}

static navi_t apply(struct navi_procedure *proc, navi_t args, navi_env_t env)
{
	navi_env_t new_env;
	if (!navi_arity_satisfied(proc, navi_list_length(args)))
		navi_arity_error(env, proc->name);
	if (proc->flags & NAVI_PROC_BUILTIN)
		return proc->c_proc(args, env);
	new_env = navi_extend_environment(proc->env, proc->args, args);
	return scm_begin(proc->body, new_env);
}

static navi_t map_eval(navi_t obj, void *data)
{
	return navi_eval(obj, data);
}

static inline navi_t make_args(navi_t args, navi_env_t env)
{
	if (navi_type(args) == NAVI_NIL)
		return navi_make_nil();
	return navi_map(args, map_eval, env);
}

static navi_t map_quote(navi_t obj, void *data)
{
	return navi_make_pair(navi_sym_quote, navi_make_pair(obj, navi_make_nil()));
}

static inline navi_t macro_call(navi_t macro, navi_t args, navi_env_t env)
{
	navi_t result;
	macro.p->type = NAVI_PROCEDURE;
	result = navi_eval(navi_make_pair(macro, navi_map(args, map_quote, NULL)), env);
	macro.p->type = NAVI_MACRO;
	return result;
}

static navi_t caselambda_call(navi_t lambda, navi_t args, navi_env_t env)
{
	int nr_args = navi_list_length(args);
	struct navi_vector *vec = navi_vector(lambda);

	for (size_t i = 0; i < vec->size; i++) {
		struct navi_procedure *proc = navi_procedure(vec->data[i]);
		if (navi_arity_satisfied(proc, nr_args))
			return apply(proc, args, env);
	}
	navi_arity_error(env, navi_make_symbol("case-lambda"));
}

static navi_t eval_call(navi_t call, navi_env_t env)
{
	navi_t expr, proc = navi_eval(navi_car(call), env);
	switch (navi_type(proc)) {
	/* special: pass args unevaluated, return result */
	case NAVI_SPECIAL:
		return apply(navi_procedure(proc), navi_cdr(call), env);
	/* procedure: pass args evaluated, return result */
	case NAVI_PROCEDURE:
		expr = make_args(navi_cdr(call), env);
		return apply(navi_procedure(proc), expr, env);
	/* macro: pass args unevaluated, return eval(result) */
	case NAVI_MACRO:
		expr = macro_call(proc, navi_cdr(call), env);
		return _eval(expr, env);
	/* escape: magic */
	case NAVI_ESCAPE:
		expr = navi_list_length(call) < 2 ? navi_make_nil() : navi_cadr(call);
		return navi_call_escape(proc, expr);
	/* caselambda: magic */
	case NAVI_CASELAMBDA:
		return caselambda_call(proc, navi_cdr(call), env);
	default: break;
	}
	navi_error(env, "call of non-procedure", navi_make_apair("value", proc));
}

static navi_t _eval(navi_t expr, navi_env_t env)
{
	navi_t val;

	switch (navi_type(expr)) {
	case NAVI_VOID:
	case NAVI_NIL:
	case NAVI_EOF:
	case NAVI_NUM:
	case NAVI_BOOL:
	case NAVI_CHAR:
	case NAVI_PORT:
	case NAVI_STRING:
	case NAVI_VECTOR:
	case NAVI_BYTEVEC:
	case NAVI_MACRO:
	case NAVI_SPECIAL:
	case NAVI_PROCEDURE:
	case NAVI_PROMISE:
	case NAVI_CASELAMBDA:
	case NAVI_ESCAPE:
	case NAVI_ENVIRONMENT:
	case NAVI_BOUNCE:
		return expr;
	case NAVI_VALUES:
		return navi_vector_ref(expr, 0);
	case NAVI_SYMBOL:
		val = navi_env_lookup(env, expr);
		if (navi_type(val) == NAVI_VOID)
			unbound_identifier(expr, env);
		return val;
	case NAVI_PAIR:
		if (!navi_is_proper_list(expr))
			navi_error(env, "malformed expression",
					navi_make_apair("expression", expr));
		return eval_call(expr, env);
	}
	return navi_unspecified();
}

navi_t navi_eval(navi_t expr, navi_env_t env)
{
	navi_t result;

	navi_scope_ref(env);
	for (;;) {
		result = _eval(expr, env);
		if (navi_type(result) != NAVI_BOUNCE)
			break;
		navi_scope_unref(env); // unref for previous bounce
		expr = bounce_object(result);
		env = bounce_env(result);
	}
	navi_scope_unref(env);
	return result;
}

DEFUN(eval, args, env, "eval", 1, 0, NAVI_ANY)
{
	return navi_eval(navi_car(args), env);
}
