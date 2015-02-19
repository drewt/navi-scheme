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

static navi_obj _eval(navi_obj expr, navi_env env);

static inline navi_obj bounce_object(navi_obj bounce)
{
	return navi_car(bounce);
}

static inline navi_env bounce_env(navi_obj bounce)
{
	return navi_cdr(bounce).p->data->env;
}

static _Noreturn void unbound_identifier(navi_obj ident, navi_env env)
{
	navi_error(env, "unbound identifier", navi_make_apair("identifier", ident));
}

static bool list_of(navi_obj list, unsigned type, bool allow_dotted_tail)
{
	navi_obj cons;

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

static inline bool can_bounce(navi_obj obj)
{
	enum navi_type type = navi_type(obj);
	return (type == NAVI_SYMBOL || type == NAVI_PAIR);
}

static navi_obj eval_tail(navi_obj tail, navi_env env)
{
	if (!can_bounce(tail))
		return tail;

	return navi_make_bounce(tail, navi_capture_env(env));
}

static bool lambda_valid(navi_obj lambda)
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

DEFSPECIAL(lambda, "lambda", 2, NAVI_PROC_VARIADIC, NAVI_ANY, NAVI_ANY)
{
	if (!lambda_valid(scm_args))
		navi_error(scm_env, "invalid lambda list");
	return navi_make_lambda(scm_arg1, navi_cdr(scm_args), scm_env);
}

DEFSPECIAL(caselambda, "case-lambda", 1, NAVI_PROC_VARIADIC, NAVI_ANY)
{
	navi_obj cons, expr;
	struct navi_vector *vec;
	unsigned i = 0;

	expr = navi_make_caselambda(navi_list_length(scm_args));
	vec = navi_vector(expr);

	navi_list_for_each(cons, scm_args) {
		if (!lambda_valid(navi_car(cons)))
			navi_error(scm_env, "invalid case-lambda list");
		vec->data[i++] = navi_make_lambda(navi_caar(cons),
				navi_cdar(cons), scm_env);
	}
	return expr;
}

static navi_obj eval_defvar(navi_obj sym, navi_obj rest, navi_env env)
{
	if (navi_type(navi_cdr(rest)) != NAVI_NIL)
		navi_arity_error(env, navi_make_symbol("define"));

	navi_scope_set(env.lexical, sym, navi_eval(navi_car(rest), env));
	return navi_unspecified();
}

static navi_obj eval_defun(navi_obj fundecl, navi_obj rest, navi_env env)
{
	navi_obj proc, name;

	if (!list_of(fundecl, NAVI_SYMBOL, true))
		navi_error(env, "invalid defun list");

	name = navi_car(fundecl);
	proc = navi_make_procedure(navi_cdr(fundecl), rest, name, env);
	navi_scope_set(env.lexical, name, proc);
	return navi_unspecified();
}

DEFSPECIAL(define, "define", 2, NAVI_PROC_VARIADIC, NAVI_ANY, NAVI_ANY)
{
	enum navi_type type;

	if (navi_list_length(scm_args) < 2)
		navi_error(scm_env, "invalid define list");

	type = navi_type(scm_arg1);
	if (type == NAVI_SYMBOL)
		return eval_defvar(scm_arg1, navi_cdr(scm_args), scm_env);
	if (type == NAVI_PAIR)
		return eval_defun(scm_arg1, navi_cdr(scm_args), scm_env);
	navi_error(scm_env, "invalid define list");
}

static void extend_with_values(navi_obj vars, navi_obj vals, navi_obj which, navi_env env)
{
	navi_obj cons;
	size_t i = 0;

	if (navi_type(vals) != NAVI_VALUES) {
		if (navi_list_length(vars) != 1)
			navi_arity_error(env, which);
		navi_scope_set(env.lexical, navi_car(vars), vals);
		return;
	}

	if ((size_t)navi_list_length(vars) != navi_vector_length(vals))
		navi_arity_error(env, which);

	navi_list_for_each(cons, vars) {
		navi_scope_set(env.lexical, navi_car(cons), navi_vector_ref(vals, i++));
	}
}

DEFSPECIAL(define_values, "define-values", 2, 0, NAVI_LIST, NAVI_ANY)
{
	extend_with_values(scm_arg1, navi_eval(scm_arg2, scm_env),
			navi_make_symbol("define-values"), scm_env);
	return navi_unspecified();
}

DEFSPECIAL(defmacro, "defmacro", 2, NAVI_PROC_VARIADIC, NAVI_PAIR, NAVI_ANY)
{
	navi_obj macro, name;

	if (!list_of(scm_arg1, NAVI_SYMBOL, true))
		navi_error(scm_env, "invalid define-macro list");

	name = navi_car(scm_arg1);
	macro = navi_make_macro(navi_cdr(scm_arg1), navi_cdr(scm_args), name, scm_env);
	navi_scope_set(scm_env.lexical, name, macro);
	return navi_unspecified();
}

DEFSPECIAL(begin, "begin", 1, NAVI_PROC_VARIADIC, NAVI_ANY)
{
	navi_obj cons, result;

	navi_list_for_each(cons, scm_args) {
		if (navi_is_last_pair(cons))
			return eval_tail(navi_car(cons), scm_env);
		result = navi_eval(navi_car(cons), scm_env);
	}
	return result;
}

static bool let_def_valid(navi_obj def)
{
	return navi_type(def) == NAVI_PAIR &&
		navi_type(navi_cdr(def)) == NAVI_PAIR &&
		navi_type(navi_cddr(def)) == NAVI_NIL &&
		navi_type(navi_car(def)) == NAVI_SYMBOL;
}

static bool let_defs_valid(navi_obj list)
{
	navi_obj cons;

	navi_list_for_each(cons, list) {
		if (!let_def_valid(navi_car(cons)))
			return false;
	}
	return navi_type(cons) == NAVI_NIL;
}

static bool let_values_def_valid(navi_obj def)
{
	return navi_type(def) == NAVI_PAIR &&
		navi_type(navi_cdr(def)) == NAVI_PAIR &&
		navi_type(navi_cddr(def)) == NAVI_NIL &&
		list_of(navi_car(def), NAVI_SYMBOL, false);
}

static bool letvals_defs_valid(navi_obj list)
{
	navi_obj cons;

	navi_list_for_each(cons, list) {
		if (!let_values_def_valid(navi_car(cons)))
			return false;
	}
	return navi_type(cons) == NAVI_NIL;
}

static navi_env let_extend_env(navi_obj def_list, navi_env env)
{
	navi_obj cons;
	navi_env new = navi_env_new_scope(env);

	navi_list_for_each(cons, def_list) {
		navi_obj defn = navi_car(cons);
		navi_obj val = navi_eval(navi_cadr(defn), env);
		navi_scope_set(new.lexical, navi_car(defn), val);
	}

	return new;
}

static navi_env sequential_let_extend_env(navi_obj def_list, navi_env env)
{
	navi_obj cons;
	navi_env new = navi_env_new_scope(env);

	navi_list_for_each(cons, def_list) {
		navi_obj defn = navi_car(cons);
		navi_obj val = navi_eval(navi_cadr(defn), new);
		navi_scope_set(new.lexical, navi_car(defn), val);
	}

	return new;
}

static navi_env letvals_extend_env(navi_obj def_list, navi_env env)
{
	navi_obj cons;
	navi_env new = navi_env_new_scope(env);

	navi_list_for_each(cons, def_list) {
		navi_obj vals = navi_eval(navi_cadar(cons), env);
		extend_with_values(navi_caar(cons), vals,
				navi_make_symbol("let-values"), env);
	}

	return new;
}

#define DEFLET(name, scmname, validate, extend) \
	DEFSPECIAL(name, scmname, 2, NAVI_PROC_VARIADIC, \
			NAVI_ANY, NAVI_ANY) \
	{ \
		navi_obj result; \
		navi_env new_env; \
		\
		if (!validate(scm_arg1)) \
			navi_error(scm_env, "invalid " scmname " list"); \
		\
		new_env = extend(scm_arg1, scm_env); \
		result = scm_begin(0, navi_cdr(scm_args), new_env); \
		navi_env_unref(new_env); \
		return result; \
	}

DEFLET(let, "let", let_defs_valid, let_extend_env)
DEFLET(sequential_let, "let*", let_defs_valid, sequential_let_extend_env)
DEFLET(let_values, "let-values", letvals_defs_valid, letvals_extend_env)

DEFSPECIAL(set, "set!", 2, 0, NAVI_SYMBOL, NAVI_ANY)
{
	struct navi_binding *binding;
	navi_obj value;

	binding = navi_env_binding(scm_env.lexical, scm_arg1);
	if (binding == NULL)
		unbound_identifier(scm_arg1, scm_env);

	value = navi_eval(scm_arg1, scm_env);
	binding->object = value;

	return navi_unspecified();
}

DEFSPECIAL(quote, "quote", 1, 0, NAVI_ANY)
{
	return scm_arg1;
}

DEFSPECIAL(unquote, "unquote", 1, 0, NAVI_ANY)
{
	return navi_eval(scm_arg1, scm_env);
}

/* FIXME: this is incredibly ugly */
static navi_obj eval_qq(navi_obj expr, navi_env env)
{
	struct navi_pair head, *last;
	navi_obj cons;

	switch (navi_type(expr)) {
	case NAVI_PAIR:
		if (navi_symbol_eq(navi_car(expr), navi_sym_unquote))
			return scm_unquote(1, navi_cdr(expr), env);

		last = &head;
		navi_list_for_each(cons, expr) {
			navi_obj elm = navi_car(cons);
			if (navi_is_pair(elm) && navi_symbol_eq(navi_car(elm), navi_sym_splice)) {
				navi_obj list = scm_unquote(1, navi_cdar(cons), env);
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

DEFSPECIAL(quasiquote, "quasiquote", 1, 0, NAVI_ANY)
{
	return eval_qq(scm_arg1, scm_env);	
}

static bool case_valid(navi_obj scase)
{
	navi_obj cons;

	if (navi_list_length(scase) < 2)
		return false;

	navi_list_for_each(cons, navi_cdr(scase)) {
		navi_obj fst = navi_car(cons);
		if (navi_list_length(fst) < 2)
			return false;
		if (!navi_symbol_eq(fst, navi_sym_else) && !navi_is_proper_list(fst))
			return false;
	}
	return true;
}

static navi_obj eval_clause(navi_obj arg, navi_obj begin, navi_env env)
{
	if (navi_symbol_eq(navi_car(begin), navi_sym_eq_lt)) {
		navi_obj proc = navi_eval(navi_cadr(begin), env);
		navi_obj call = navi_make_pair(proc, navi_make_pair(arg, navi_make_nil()));
		return _eval(call, env);
	}
	return scm_begin(0, begin, env);
}

DEFSPECIAL(case, "case", 2, NAVI_PROC_VARIADIC, NAVI_ANY, NAVI_ANY)
{
	navi_obj test, cons, inner;

	if (!case_valid(scm_args))
		navi_error(scm_env, "invalid case list");

	test = navi_eval(scm_arg1, scm_env);
	navi_list_for_each(cons, navi_cdr(scm_args)) {
		navi_obj fst = navi_car(cons);
		if (navi_symbol_eq(navi_car(fst), navi_sym_else))
			return eval_clause(test, navi_cdr(fst), scm_env);
		navi_list_for_each(inner, navi_car(fst)) {
			if (navi_eqvp(navi_car(inner), test))
				return eval_clause(test, navi_cdr(fst), scm_env);
		}
	}
	return navi_unspecified();
}

static bool cond_valid(navi_obj cond)
{
	navi_obj cons;

	navi_list_for_each(cons, cond) {
		int length;
		navi_obj fst = navi_car(cons);
		if (!navi_is_proper_list(fst))
			return false;
		if ((length = navi_list_length(fst)) < 2)
			return false;
		if (navi_symbol_eq(navi_cadr(fst), navi_sym_eq_lt) && length != 3)
			return false;
	}
	return navi_type(cons) == NAVI_NIL;
}

static navi_obj scm_cond_clause(navi_obj test, navi_obj clause, navi_env env)
{
	if (navi_symbol_eq(navi_car(clause), navi_sym_eq_lt)) {
		navi_obj proc = navi_eval(navi_cadr(clause), env);
		navi_obj call = navi_make_pair(proc, navi_make_pair(test, navi_make_nil()));
		return _eval(call, env);
	}
	return scm_begin(0, clause, env);
}

DEFSPECIAL(cond, "cond", 1, NAVI_PROC_VARIADIC, NAVI_ANY)
{
	navi_obj cons;

	if (!cond_valid(scm_args))
		navi_error(scm_env, "invalid cond list");

	navi_list_for_each(cons, scm_args) {
		navi_obj test, fst = scm_arg1;

		if (navi_symbol_eq(navi_car(fst), navi_sym_else))
			return eval_clause(navi_make_bool(true), navi_cdr(fst), scm_env);

		test = navi_eval(navi_car(fst), scm_env);
		if (navi_is_true(test))
			return scm_cond_clause(test, navi_cdr(fst), scm_env);
	}
	return navi_unspecified();
}

DEFSPECIAL(if, "if", 2, NAVI_PROC_VARIADIC, NAVI_ANY, NAVI_ANY)
{
	navi_obj test;

	if (scm_nr_args > 3)
		navi_arity_error(scm_env, navi_make_symbol("if"));

	test = navi_eval(scm_arg1, scm_env);
	if (navi_is_true(test))
		return eval_tail(scm_arg2, scm_env);
	if (scm_nr_args == 3)
		return eval_tail(scm_arg3, scm_env);
	return navi_unspecified();
}

DEFSPECIAL(and, "and", 0, NAVI_PROC_VARIADIC)
{
	navi_obj cons;
	navi_list_for_each(cons, scm_args) {
		if (navi_is_last_pair(cons))
			return eval_tail(navi_car(cons), scm_env);
		if (!navi_is_true(navi_eval(navi_car(cons), scm_env)))
			return navi_make_bool(false);
	}
	return navi_make_bool(true);
}

DEFSPECIAL(or, "or", 0, NAVI_PROC_VARIADIC)
{
	navi_obj cons;
	navi_list_for_each(cons, scm_args) {
		if (navi_is_last_pair(cons))
			return eval_tail(navi_car(cons), scm_env);
		if (navi_is_true(navi_eval(navi_car(cons), scm_env)))
			return navi_make_bool(true);
	}
	return navi_make_bool(false);
}

DEFSPECIAL(delay, "delay", 1, 0, NAVI_ANY)
{
	return navi_make_promise(scm_arg1, scm_env);
}

DEFUN(force, "force", 1, 0, NAVI_PROCEDURE)
{
	struct navi_procedure *proc = navi_procedure(scm_arg1);
	navi_env env = { .lexical = proc->env, .dynamic = scm_env.dynamic };
	navi_obj r = navi_eval(navi_make_pair(navi_sym_begin, proc->body), env);
	proc->body = navi_make_pair(navi_sym_quote, navi_make_pair(r, navi_make_nil()));
	return r;
}

static navi_obj do_apply(struct navi_procedure *proc, unsigned nr_args,
		navi_obj args, navi_env env)
{
	navi_env new_env;
	navi_obj result;
	if (proc->flags & NAVI_PROC_BUILTIN)
		return proc->c_proc(nr_args, args, env);
	new_env = navi_extend_environment(env, proc->args, args);
	result = scm_begin(0, proc->body, new_env);
	navi_env_unref(new_env);
	return result;
}

static unsigned check_apply(struct navi_procedure *proc, navi_obj args,
		navi_env env)
{
	unsigned nr_args = navi_list_length(args);
	if (!navi_arity_satisfied(proc, nr_args))
		navi_arity_error(env, proc->name);
	if (proc->types) {
		navi_obj cons;
		unsigned i = 0;
		navi_list_for_each(cons, args) {
			if (i >= proc->arity)
				break;
			switch (proc->types[i]) {
			case NAVI_LIST:
				navi_type_check_list(navi_car(cons), env);
				break;
			case NAVI_BYTE:
				navi_type_check_byte(navi_car(cons), env);
				break;
			case NAVI_ANY:
				break;
			default:
				navi_type_check(navi_car(cons), proc->types[i], env);
			}
			i++;
		}
	}
	return nr_args;
}

navi_obj _navi_apply(struct navi_procedure *proc, navi_obj args, navi_env env)
{
	return do_apply(proc, check_apply(proc, args, env), args, env);
}

static navi_obj map_eval(navi_obj obj, void *data)
{
	return navi_eval(obj, *((navi_env*)data));
}

static inline navi_obj make_args(navi_obj args, navi_env env)
{
	if (navi_type(args) == NAVI_NIL)
		return navi_make_nil();
	return navi_map(args, map_eval, &env);
}

static navi_obj map_quote(navi_obj obj, void *data)
{
	return navi_make_pair(navi_sym_quote, navi_make_pair(obj, navi_make_nil()));
}

static inline navi_obj macro_call(navi_obj macro, navi_obj args, navi_env env)
{
	navi_obj result;
	macro.p->type = NAVI_PROCEDURE;
	result = navi_eval(navi_make_pair(macro, navi_map(args, map_quote, NULL)), env);
	macro.p->type = NAVI_MACRO;
	return result;
}

static navi_obj caselambda_call(navi_obj lambda, navi_obj args, navi_env env)
{
	int nr_args = navi_list_length(args);
	struct navi_vector *vec = navi_vector(lambda);

	for (size_t i = 0; i < vec->size; i++) {
		struct navi_procedure *proc = navi_procedure(vec->data[i]);
		if (navi_arity_satisfied(proc, nr_args))
			return navi_apply(proc, args, env);
	}
	navi_arity_error(env, navi_make_symbol("case-lambda"));
}

static navi_obj eval_call(navi_obj call, navi_env env)
{
	navi_obj expr, proc = navi_eval(navi_car(call), env);
	switch (navi_type(proc)) {
	/* special: pass args unevaluated, return result */
	case NAVI_SPECIAL:
		return _navi_apply(navi_procedure(proc), navi_cdr(call), env);
	/* procedure: pass args evaluated, return result */
	case NAVI_PROCEDURE:
		expr = make_args(navi_cdr(call), env);
		return navi_apply(navi_procedure(proc), expr, env);
	/* macro: pass args unevaluated, return eval(result) */
	case NAVI_MACRO:
		expr = macro_call(proc, navi_cdr(call), env);
		return _eval(expr, env);
	/* escape: magic */
	case NAVI_ESCAPE:
		expr = navi_list_length(call) < 2 ? navi_make_nil() : navi_cadr(call);
		return navi_call_escape(proc, expr, env);
	/* caselambda: magic */
	case NAVI_CASELAMBDA:
		return caselambda_call(proc, navi_cdr(call), env);
	case NAVI_PARAMETER:
		if (!navi_is_nil(navi_cdr(call)))
			navi_arity_error(env, navi_car(proc));
		return navi_parameter_lookup(proc, env);
	default: break;
	}
	navi_error(env, "call of non-procedure", navi_make_apair("value", proc));
}

static navi_obj _eval(navi_obj expr, navi_env env)
{
	navi_obj val;

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
	case NAVI_PARAMETER:
	case NAVI_ENVIRONMENT:
	case NAVI_BOUNCE:
		return expr;
	case NAVI_VALUES:
		return navi_vector_ref(expr, 0);
	case NAVI_SYMBOL:
		val = navi_env_lookup(env.lexical, expr);
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

navi_obj navi_eval(navi_obj expr, navi_env env)
{
	navi_obj result;

	navi_env_ref(env);
	for (;;) {
		result = _eval(expr, env);
		if (navi_type(result) != NAVI_BOUNCE)
			break;
		navi_env_unref(env); // unref for previous bounce
		expr = bounce_object(result);
		env = bounce_env(result);
	}
	navi_env_unref(env);
	return result;
}

DEFUN(eval, "eval", 1, 0, NAVI_ANY)
{
	return navi_eval(scm_arg1, scm_env);
}
