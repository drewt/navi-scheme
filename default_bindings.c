/* Copyright 2014-2015 Drew Thoreson
 *
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/.
 */

static struct navi_scope *internal_env = NULL;

navi_obj navi_get_internal(navi_obj symbol, navi_env env)
{
	struct navi_binding *binding = navi_env_binding(internal_env, symbol);
	if (unlikely(!binding))
		navi_error(env, "no internal binding",
				navi_make_apair("symbol", symbol));

	// make sure functions/etc. are bound in *global* environment
	return navi_from_spec(binding->object.v, navi_get_global_env(env));
}

/*
 * The top-level exception handler: prints a message and returns to the REPL.
 */
DEFUN(toplevel_exn, "#exn", 1, 0, NAVI_ANY)
{
	navi_obj cont;

	navi_write(scm_arg1, scm_env);
	putchar('\n');

	cont = navi_env_lookup(scm_env.lexical, navi_sym_repl);
	if (navi_type(cont) != NAVI_ESCAPE)
		navi_die("#repl not bound to continuation");

	navi_scope_set(scm_env.dynamic, navi_sym_current_exn,
			navi_from_spec(&SCM_DECL(toplevel_exn), scm_env));
	longjmp(navi_escape(cont)->state, 1);
}

DEFUN(check_exception_handler, "#check-exception-handler", 1, 0, NAVI_PROCEDURE)
{
	navi_check_arity(scm_arg1, 1, scm_env);
	return scm_arg1;
}

DEFPARAM(current_exception_handler, "#current-exception-handler",
		toplevel_exn, check_exception_handler);

#define DECL_SPEC(name) &SCM_DECL(name)
static const struct navi_spec *builtin_objects[] = {
	DECL_SPEC(current_exception_handler),

	DECL_SPEC(lambda),
	DECL_SPEC(caselambda),
	DECL_SPEC(define),
	DECL_SPEC(define_syntax),
	DECL_SPEC(define_values),
	DECL_SPEC(defmacro),
	DECL_SPEC(define_library),
	DECL_SPEC(import),
	DECL_SPEC(begin),
	DECL_SPEC(include),
	DECL_SPEC(include_ci),
	DECL_SPEC(let),
	DECL_SPEC(sequential_let),
	DECL_SPEC(let_values),
	DECL_SPEC(set),
	DECL_SPEC(quote),
	DECL_SPEC(unquote),
	DECL_SPEC(quasiquote),
	DECL_SPEC(case),
	DECL_SPEC(cond),
	DECL_SPEC(if),
	DECL_SPEC(and),
	DECL_SPEC(or),
	DECL_SPEC(delay),
	DECL_SPEC(make_parameter),
	DECL_SPEC(parameterize),

	DECL_SPEC(command_line),
	DECL_SPEC(emergency_exit),
	DECL_SPEC(exit),
	DECL_SPEC(get_environment_variable),
	DECL_SPEC(get_environment_variables),
	DECL_SPEC(current_second),
	DECL_SPEC(current_jiffy),
	DECL_SPEC(jiffies_per_second),

	DECL_SPEC(eqvp),
	DECL_SPEC(eqp),
	DECL_SPEC(equalp),

	DECL_SPEC(eval),
	DECL_SPEC(apply),
	DECL_SPEC(force),
	DECL_SPEC(promisep),
	DECL_SPEC(make_promise),
	DECL_SPEC(call_ec),
	DECL_SPEC(values),
	DECL_SPEC(call_with_values),
	DECL_SPEC(with_exception_handler),
	DECL_SPEC(raise),
	DECL_SPEC(raise_continuable),
	DECL_SPEC(error),
	DECL_SPEC(error_objectp),
	DECL_SPEC(error_object_message),
	DECL_SPEC(error_object_irritants),
	DECL_SPEC(read_errorp),

	DECL_SPEC(add),
	DECL_SPEC(sub),
	DECL_SPEC(mul),
	DECL_SPEC(div),
	DECL_SPEC(quotient),
	DECL_SPEC(remainder),
	DECL_SPEC(lt),
	DECL_SPEC(gt),
	DECL_SPEC(lte),
	DECL_SPEC(gte),
	DECL_SPEC(numeq),
	DECL_SPEC(zerop),
	DECL_SPEC(positivep),
	DECL_SPEC(negativep),
	DECL_SPEC(oddp),
	DECL_SPEC(evenp),
	DECL_SPEC(number_to_string),
	DECL_SPEC(string_to_number),

	DECL_SPEC(not),
	DECL_SPEC(booleanp),
	DECL_SPEC(boolean_eq),

	DECL_SPEC(charp),
	DECL_SPEC(char_lt),
	DECL_SPEC(char_gt),
	DECL_SPEC(char_eq),
	DECL_SPEC(char_lte),
	DECL_SPEC(char_gte),
	DECL_SPEC(char_ci_lt),
	DECL_SPEC(char_ci_gt),
	DECL_SPEC(char_ci_eq),
	DECL_SPEC(char_ci_lte),
	DECL_SPEC(char_ci_gte),
	DECL_SPEC(char_upcase),
	DECL_SPEC(char_downcase),

	DECL_SPEC(cons),
	DECL_SPEC(car),
	DECL_SPEC(cdr),
	DECL_SPEC(set_car),
	DECL_SPEC(set_cdr),
	DECL_SPEC(pairp),
	DECL_SPEC(listp),
	DECL_SPEC(nullp),
	DECL_SPEC(list),
	DECL_SPEC(make_list),
	DECL_SPEC(length),
	DECL_SPEC(append),
	DECL_SPEC(reverse),
	DECL_SPEC(list_tail),
	DECL_SPEC(list_ref),
	DECL_SPEC(list_set),
	DECL_SPEC(memq),
	DECL_SPEC(memv),
	DECL_SPEC(member),
	DECL_SPEC(assq),
	DECL_SPEC(assv),
	DECL_SPEC(assoc),
	DECL_SPEC(list_copy),
	DECL_SPEC(for_each),
	DECL_SPEC(map),

	DECL_SPEC(stringp),
	DECL_SPEC(make_string),
	DECL_SPEC(string_length),
	DECL_SPEC(string),
	DECL_SPEC(string_ref),
	DECL_SPEC(string_set),
	DECL_SPEC(string_lt),
	DECL_SPEC(string_gt),
	DECL_SPEC(string_eq),
	DECL_SPEC(string_lte),
	DECL_SPEC(string_gte),
	DECL_SPEC(string_ci_lt),
	DECL_SPEC(string_ci_gt),
	DECL_SPEC(string_ci_eq),
	DECL_SPEC(string_ci_lte),
	DECL_SPEC(string_ci_gte),
	DECL_SPEC(string_upcase),
	DECL_SPEC(string_downcase),
	DECL_SPEC(string_foldcase),
	DECL_SPEC(substring),
	DECL_SPEC(string_append),
	DECL_SPEC(string_to_list),
	DECL_SPEC(list_to_string),
	DECL_SPEC(string_to_vector),
	DECL_SPEC(vector_to_string),
	DECL_SPEC(string_fill),
	DECL_SPEC(string_copy),
	DECL_SPEC(string_copy_to),

	DECL_SPEC(vectorp),
	DECL_SPEC(make_vector),
	DECL_SPEC(vector),
	DECL_SPEC(vector_length),
	DECL_SPEC(vector_ref),
	DECL_SPEC(vector_set),
	DECL_SPEC(vector_to_list),
	DECL_SPEC(list_to_vector),
	DECL_SPEC(vector_fill),
	DECL_SPEC(vector_copy),
	DECL_SPEC(vector_copy_to),
	DECL_SPEC(vector_for_each),
	DECL_SPEC(vector_map),
	DECL_SPEC(vector_map_ip),

	DECL_SPEC(bytevectorp),
	DECL_SPEC(make_bytevector),
	DECL_SPEC(bytevector),
	DECL_SPEC(bytevector_length),
	DECL_SPEC(bytevector_u8_ref),
	DECL_SPEC(bytevector_u8_set),
	DECL_SPEC(bytevector_append),
	DECL_SPEC(bytevector_copy),
	DECL_SPEC(bytevector_copy_to),
	DECL_SPEC(utf8_to_string),
	DECL_SPEC(string_to_utf8),

	DECL_SPEC(input_portp),
	DECL_SPEC(output_portp),
	DECL_SPEC(portp),
	DECL_SPEC(input_port_openp),
	DECL_SPEC(output_port_openp),
	DECL_SPEC(current_input_port),
	DECL_SPEC(current_output_port),
	DECL_SPEC(current_error_port),
	DECL_SPEC(open_input_file),
	DECL_SPEC(open_output_file),
	DECL_SPEC(open_input_string),
	DECL_SPEC(open_output_string),
	DECL_SPEC(get_output_string),
	DECL_SPEC(close_port),
	DECL_SPEC(close_input_port),
	DECL_SPEC(close_output_port),
	DECL_SPEC(eof_objectp),
	DECL_SPEC(eof_object),
	DECL_SPEC(read_u8),
	DECL_SPEC(peek_u8),
	DECL_SPEC(read_char),
	DECL_SPEC(peek_char),
	DECL_SPEC(read),
	DECL_SPEC(write_u8),
	DECL_SPEC(write_char),
	DECL_SPEC(write_string),
	DECL_SPEC(write),
	DECL_SPEC(display),
	DECL_SPEC(newline),

	DECL_SPEC(env_count),
	DECL_SPEC(gc_collect),
	DECL_SPEC(gc_count),
	NULL
};

void navi_internal_init(void)
{
	internal_env = _navi_make_scope();
	for (int i = 0; builtin_objects[i]; i++) {
		navi_obj symbol = navi_make_symbol(builtin_objects[i]->ident);
		navi_obj object = (navi_obj) { .v = (void*) builtin_objects[i] };
		navi_scope_set(internal_env, symbol, object);
	}
}
