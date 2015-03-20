/* Copyright 2014-2015 Drew Thoreson
 *
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/.
 */

static inline navi_obj eval_tail(navi_obj tail, navi_env env)
{
	return navi_make_bounce(tail, env);
}

static bool lambda_valid(navi_obj lambda)
{
	if (navi_type(lambda) != NAVI_PAIR)
		return false;
	if (navi_type(navi_cdr(lambda)) != NAVI_PAIR)
		return false;
	if (navi_type(navi_car(lambda)) == NAVI_SYMBOL)
		return true;
	if (!navi_is_list_of(navi_car(lambda), NAVI_SYMBOL, true))
		return false;
	return true;
}

DEFSPECIAL(lambda, "lambda", 2, NAVI_PROC_VARIADIC, NAVI_ANY, NAVI_ANY)
{
	if (unlikely(!lambda_valid(scm_args)))
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
		if (unlikely(!lambda_valid(navi_car(cons))))
			navi_error(scm_env, "invalid case-lambda list");
		vec->data[i++] = navi_make_lambda(navi_caar(cons),
				navi_cdar(cons), scm_env);
	}
	return expr;
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

/*
 * Load the given file into the given environment.
 * XXX: The function may return a bounce object--take care not to bounce with
 *      the wrong environment.
 */
navi_obj navi_load(navi_obj filename, bool ci, navi_env in_env, navi_env out_env)
{
	navi_obj fst, snd;
	// FIXME: need to protect unbound object from gc somehow...
	navi_obj port_obj = navi_open_input_file(filename, out_env);
	struct navi_port *port = navi_port(port_obj);
	struct navi_guard *guard = navi_gc_guard(port_obj, out_env);

	navi_port_set_fold_case(port, ci);
	// read/eval until EOF with 1 expr lookahead for tail call
	fst = navi_read(port, in_env);
	// special case: if the file is empty, return void and not #!eof
	if (navi_is_eof(fst)) {
		navi_close_input_port(port, out_env);
		navi_gc_unguard(guard);
		return navi_make_void();
	}
	while (!navi_is_eof((snd = navi_read(port, in_env)))) {
		navi_eval(fst, in_env);
		fst = snd;
	}
	navi_close_input_port(port, out_env);
	navi_gc_unguard(guard);
	return eval_tail(fst, in_env);
}

static navi_obj navi_include(navi_obj args, bool ci, navi_env env)
{
	navi_obj cons, result;
	navi_list_for_each(cons, args) {
		navi_type_check(navi_car(cons), NAVI_STRING, env);
	}
	navi_list_for_each(cons, args) {
		result = navi_load(navi_car(cons), ci, env, env);
		if (!navi_is_last_pair(cons))
			result = navi_force_tail(result, env);
	}
	return result;
}

DEFSPECIAL(include, "include", 1, NAVI_PROC_VARIADIC, NAVI_ANY)
{
	return navi_include(scm_args, false, scm_env);
}

DEFSPECIAL(include_ci, "include-ci", 1, NAVI_PROC_VARIADIC, NAVI_ANY)
{
	return navi_include(scm_args, true, scm_env);
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
			return scm_unquote(1, navi_cdr(expr), env, NULL);

		last = &head;
		navi_list_for_each(cons, expr) {
			navi_obj elm = navi_car(cons);
			if (navi_is_pair(elm) && navi_symbol_eq(navi_car(elm), navi_sym_splice)) {
				navi_obj list = scm_unquote(1, navi_cdar(cons), env, NULL);
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
		return eval_tail(call, env);
	}
	return scm_begin(0, begin, env, NULL);
}

DEFSPECIAL(case, "case", 2, NAVI_PROC_VARIADIC, NAVI_ANY, NAVI_ANY)
{
	navi_obj test, cons, inner;

	if (unlikely(!case_valid(scm_args)))
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
	return navi_is_nil(cons);
}

DEFSPECIAL(cond, "cond", 1, NAVI_PROC_VARIADIC, NAVI_ANY)
{
	navi_obj cons;

	if (unlikely(!cond_valid(scm_args)))
		navi_error(scm_env, "invalid cond list");

	navi_list_for_each(cons, scm_args) {
		navi_obj test, fst = navi_car(cons);

		if (navi_symbol_eq(navi_car(fst), navi_sym_else))
			return eval_clause(navi_make_bool(true), navi_cdr(fst), scm_env);

		test = navi_eval(navi_car(fst), scm_env);
		if (navi_is_true(test))
			return eval_clause(test, navi_cdr(fst), scm_env);
	}
	return navi_unspecified();
}

DEFSPECIAL(if, "if", 2, NAVI_PROC_VARIADIC, NAVI_ANY, NAVI_ANY)
{
	navi_obj test;

	if (unlikely(scm_nr_args > 3))
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
	navi_obj result;
	if (navi_proc_is_builtin(proc)) {
		struct navi_guard *guard = navi_gc_guard(args, env);
		result = proc->c_proc(nr_args, args, env, proc);
		navi_gc_unguard(guard);
	} else {
		navi_env new = navi_extend_environment(env, proc->args, args);
		result = scm_begin(0, proc->body, new, NULL);
		navi_env_unref(new);
	}
	struct navi_guard *guard = navi_gc_guard(result, env);
	navi_gc_check();
	navi_gc_unguard(guard);
	return result;
}

static unsigned check_apply(struct navi_procedure *proc, navi_obj args,
		navi_env env)
{
	unsigned nr_args = navi_list_length(args);
	if (unlikely(!navi_arity_satisfied(proc, nr_args)))
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
			case NAVI_PROPER_LIST:
				navi_type_check_proper_list(navi_car(cons), env);
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

static navi_obj procedure_call(struct navi_procedure *proc, navi_obj args,
		navi_env env)
{
	navi_obj cons;
	struct navi_pair head, *ptr = &head;
	struct navi_guard *guard = NULL;

	navi_list_for_each(cons, args) {
		ptr->cdr = navi_make_pair(navi_make_void(), navi_make_nil());
		if (ptr == &head)
			guard = navi_gc_guard(ptr->cdr, env);
		ptr = navi_pair(ptr->cdr);
		ptr->car = navi_eval(navi_car(cons), env);
	}
	ptr->cdr = navi_make_nil();

	navi_gc_unguard(guard);
	return navi_apply(proc, head.cdr, env);
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
	navi_obj obj, proc = navi_eval(navi_car(call), env);
	struct navi_guard *guard = navi_gc_guard(proc, env);
	switch (navi_type(proc)) {
	// special: pass args unevaluated, return result
	case NAVI_SPECIAL:
		obj = _navi_apply(navi_procedure(proc), navi_cdr(call), env);
		break;
	// procedure: pass args evaluated, return result
	case NAVI_PROCEDURE:
		obj = procedure_call(navi_procedure(proc), navi_cdr(call), env);
		break;
	// macro: pass args unevaluated, return eval(result)
	case NAVI_MACRO:
		obj = eval_tail(_navi_apply(navi_procedure(proc), navi_cdr(call), env), env);
		break;
	// escape: magic
	case NAVI_ESCAPE:
		obj = navi_list_length(call) < 2 ? navi_make_nil() : navi_cadr(call);
		obj = navi_call_escape(proc, obj, env);
		break;
	// caselambda: magic
	case NAVI_CASELAMBDA:
		obj = caselambda_call(proc, navi_cdr(call), env);
		break;
	case NAVI_PARAMETER:
		if (unlikely(!navi_is_nil(navi_cdr(call))))
			navi_arity_error(env, navi_car(proc));
		obj = navi_parameter_lookup(proc, env);
		break;
	default:
		navi_error(env, "call of non-procedure", navi_make_apair("value", proc));
	}
	navi_gc_unguard(guard);
	return obj;
}

static navi_obj _eval(navi_obj expr, navi_env env)
{
	navi_obj val;

	switch (navi_type(expr)) {
	case NAVI_VOID:
	case NAVI_NIL:
	case NAVI_EOF:
	case NAVI_FIXNUM:
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
		return expr;
	case NAVI_THUNK:
	case NAVI_BOUNCE:
		return _eval(navi_thunk(expr)->expr, navi_thunk(expr)->env);
	case NAVI_VALUES:
		return navi_vector_ref(expr, 0);
	case NAVI_SYMBOL:
		val = navi_env_lookup(env.lexical, expr);
		if (unlikely(navi_type(val) == NAVI_VOID))
			navi_unbound_identifier_error(env, expr);
		return val;
	case NAVI_PAIR:
		if (unlikely(!navi_is_proper_list(expr)))
			navi_error(env, "malformed expression",
					navi_make_apair("expression", expr));
		return eval_call(expr, env);
	case NAVI_TRAP:
		navi_die("trap!");
	}
	return navi_unspecified();
}

navi_obj navi_eval(navi_obj expr, navi_env env)
{
	navi_env_ref(env);
	do {
		struct navi_guard *guard = navi_gc_guard(expr, env);
		expr = _eval(expr, env);
		navi_gc_unguard(guard);
	} while (navi_is_bounce(expr));
	navi_env_unref(env);
	return expr;
}

DEFUN(eval, "eval", 1, 0, NAVI_ANY)
{
	return navi_eval(scm_arg1, scm_env);
}
