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

#include "navi.h"

DECLARE(toplevel_exn);
#define TOPLEVEL_EXN_INDEX 0

static navi_obj init_input_port(const struct navi_spec *spec)
{
	return navi_make_file_input_port(*((FILE**)spec->ptr));
}

static navi_obj init_output_port(const struct navi_spec *spec)
{
	return navi_make_file_output_port(*((FILE**)spec->ptr));
}

static struct navi_spec stdin_spec = {
	.ident = "#current-input-port",
	.ptr = &stdin,
	.init = init_input_port,
};

static struct navi_spec stdout_spec = {
	.ident = "#current-output-port",
	.ptr = &stdout,
	.init = init_output_port,
};

static struct navi_spec stderr_spec = {
	.ident = "#current-error-port",
	.ptr = &stderr,
	.init = init_output_port,
};

#define DECL_SPEC(name) &scm_decl_##name
static const struct navi_spec *default_bindings[] = {
	[TOPLEVEL_EXN_INDEX] = DECL_SPEC(toplevel_exn),

	&stdin_spec,
	&stdout_spec,
	&stderr_spec,

	DECL_SPEC(lambda),
	DECL_SPEC(caselambda),
	DECL_SPEC(define),
	DECL_SPEC(define_values),
	DECL_SPEC(defmacro),
	DECL_SPEC(begin),
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

	DECL_SPEC(eqvp),
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
	DECL_SPEC(pairp),
	DECL_SPEC(listp),
	DECL_SPEC(nullp),
	DECL_SPEC(list),
	DECL_SPEC(length),
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
	DECL_SPEC(string_fill),
	DECL_SPEC(string_copy),
	DECL_SPEC(string_copy_to),
	DECL_SPEC(string_map),
	DECL_SPEC(string_map_ip),

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

/*
 * The top-level exception handler: prints a message and returns to the REPL.
 */
DEFUN(toplevel_exn, "#exn", 1, 0, NAVI_ANY)
{
	navi_obj cont;

	navi_write(scm_arg1, scm_env);
	putchar('\n');

	cont = navi_env_lookup(scm_env, navi_sym_repl);
	if (navi_type(cont) != NAVI_ESCAPE)
		navi_die("#repl not bound to continuation");

	navi_scope_set(scm_env.lexical, navi_sym_exn,
			navi_from_spec(default_bindings[TOPLEVEL_EXN_INDEX], scm_env));
	longjmp(navi_escape(cont)->state, 1);
}


