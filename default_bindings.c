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

/* FUNCTION(symbol cname, procedure _fn, int ary, bool var)
 * @cname:	the (C) name of the created function object
 * @_fn:	the function to which the object refers
 * @ary:	the (minimum) arity of the function
 * @var:	non-zero if the function is variadic
 */
#define FUNCTION_TYPE(scmname, _fn, ary, var, _type) \
{ \
	.type = _type, \
	.fun = { \
		.fn       = _fn, \
		.arity    = ary, \
		.variadic = var, \
		.builtin  = 1, \
	}, \
	.ident = scmname, \
}

#define SPECIAL(scmname, _fn, ary, var) \
	FUNCTION_TYPE(scmname, _fn, ary, var, NAVI_SPECIAL)

#define FUNCTION(scmname, _fn, ary, var) \
	FUNCTION_TYPE(scmname, _fn, ary, var, NAVI_FUNCTION)

#define MACRO(scmname, _fn, ary, var) \
	FUNCTION_TYPE(scmname, _fn, ary, var, NAVI_MACRO)

#define NUMBER(scmname, val) \
{ \
	.type = NAVI_NUM, \
	.num = val, \
	.ident = scmname, \
}

#define BOOLEAN(scmname, val) \
{ \
	.type = NAVI_BOOL, \
	.num = val, \
	.ident = scmname, \
}

static DECLARE(scm_toplevel_exn);
#define TOPLEVEL_EXN_INDEX 0

static struct navi_spec default_bindings[] = {
	[TOPLEVEL_EXN_INDEX] = FUNCTION("#exn",  scm_toplevel_exn, 1, 0),

	SPECIAL("lambda",        scm_lambda,         2, 1),
	SPECIAL("case-lambda",   scm_caselambda,     2, 1),
	SPECIAL("define",        scm_define,         2, 1),
	SPECIAL("define-values", scm_define_values,  2, 1),
	SPECIAL("define-macro",  scm_defmacro,       2, 1),
	SPECIAL("begin",         scm_begin,          1, 1),
	SPECIAL("let",           scm_let,            2, 1),
	SPECIAL("let*",          scm_sequential_let, 2, 1),
	SPECIAL("letrec",        scm_let,            2, 1),
	SPECIAL("letrec*",       scm_sequential_let, 2, 1),
	SPECIAL("let-values",    scm_let_values,     2, 1),
	SPECIAL("set!",          scm_set,            2, 0),
	SPECIAL("quote",         scm_quote,          1, 0),
	SPECIAL("quasiquote",    scm_quasiquote,     1, 0),
	SPECIAL("case",          scm_case,           2, 1),
	SPECIAL("cond",          scm_cond,           1, 1),
	SPECIAL("if",            scm_if,             2, 1),
	SPECIAL("and",           scm_and,            0, 1),
	SPECIAL("or",            scm_or,             0, 1),
	SPECIAL("delay",         scm_delay,          1, 0),

	FUNCTION("gensym", scm_gensym, 0, 0),

	FUNCTION("eval",        scm_eval,          1, 0),
	FUNCTION("apply",       scm_apply,         2, 1),
	FUNCTION("map",         scm_map,           2, 0),
	FUNCTION("string-map",  scm_string_map,    2, 0),
	FUNCTION("string-map!", scm_string_map_ip, 2, 0),
	FUNCTION("vector-map",  scm_vector_map,    2, 0),
	FUNCTION("vector-map!", scm_vector_map_ip, 2, 0),

	FUNCTION("force",        scm_force,        1, 0),
	FUNCTION("promise?",     scm_promisep,     1, 0),
	FUNCTION("make-promise", scm_make_promise, 1, 0),

	FUNCTION("call/ec", scm_call_ec, 0, 1),
	FUNCTION("values", scm_values, 1, 1),
	FUNCTION("call-with-values", scm_call_with_values, 2, 0),
	FUNCTION("with-exception-handler", scm_with_exception_handler, 2, 0),
	FUNCTION("raise", scm_raise, 1, 0),
	FUNCTION("raise-continuable", scm_raise_continuable, 1, 0),
	FUNCTION("error", scm_error, 1, 1),
	FUNCTION("error-object?", scm_error_objectp, 1, 0),
	FUNCTION("error-object-message", scm_error_object_message, 1, 0),
	FUNCTION("error-object-irritants", scm_error_object_irritants, 1, 0),
	FUNCTION("read-error?", scm_read_errorp, 1, 0),

	FUNCTION("input-port?",             scm_input_portp,         1, 0),
	FUNCTION("output-port?",            scm_output_portp,        1, 0),
	FUNCTION("textual-port?",           scm_portp,               1, 0),
	FUNCTION("binary-port?",            scm_portp,               1, 0),
	FUNCTION("port?",                   scm_portp,               1, 0),
	FUNCTION("input-port-open?",        scm_input_port_openp,    1, 0),
	FUNCTION("output-port-open?",       scm_output_port_openp,   1, 0),
	FUNCTION("current-input-port",      scm_current_input_port,  0, 0),
	FUNCTION("current-output-port",     scm_current_output_port, 0, 0),
	FUNCTION("current-error-port",      scm_current_error_port,  0, 0),
	FUNCTION("open-input-file",         scm_open_input_file,     1, 0),
	FUNCTION("open-binary-input-file",  scm_open_input_file,     1, 0),
	FUNCTION("open-output-file",        scm_open_output_file,    1, 0),
	FUNCTION("open-binary-output-file", scm_open_output_file,    1, 0),
	FUNCTION("open-input-string",       scm_open_input_string,   1, 0),
	FUNCTION("open-output-string",      scm_open_output_string,  0, 0),
	FUNCTION("get-output-string",       scm_get_output_string,   1, 0),
	FUNCTION("close-port",              scm_close_port,          1, 0),
	FUNCTION("close-input-port",        scm_close_input_port,    1, 0),
	FUNCTION("close-output-port",       scm_close_output_port,   1, 0),
	FUNCTION("eof-object?",             scm_eof_objectp,         1, 0),
	FUNCTION("eof-object",              scm_eof_object,          0, 0),
	FUNCTION("read-u8",                 scm_read_u8,             0, 1),
	FUNCTION("peek-u8",                 scm_peek_u8,             0, 1),
	FUNCTION("read-char",               scm_read_char,           0, 1),
	FUNCTION("peek-char",               scm_peek_char,           0, 1),
	FUNCTION("read",                    scm_read,                0, 1),
	FUNCTION("write-u8",                scm_write_u8,            1, 1),
	FUNCTION("write-char",              scm_write_char,          1, 1),
	FUNCTION("write-string",            scm_write_string,        1, 1),
	FUNCTION("write",                   scm_write,               1, 1),
	FUNCTION("display",                 scm_display,             1, 1),
	FUNCTION("newline",                 scm_newline,             0, 1),

	FUNCTION("eqv?", scm_eqvp, 2, 0),

	FUNCTION("+", scm_add, 0, 1),
	FUNCTION("-", scm_sub, 1, 1),
	FUNCTION("*", scm_mul, 0, 1),
	FUNCTION("/", scm_div, 1, 1),
	FUNCTION("quotient",  scm_quotient,  2, 0),
	FUNCTION("remainder", scm_remainder, 2, 0),

	FUNCTION("<",  scm_lt,    1, 1),
	FUNCTION(">",  scm_gt,    1, 1),
	FUNCTION("<=", scm_lte,   1, 1),
	FUNCTION(">=", scm_gte,   1, 1),
	FUNCTION("=",  scm_numeq, 1, 1),

	FUNCTION("zero?",     scm_zerop,     1, 0),
	FUNCTION("positive?", scm_positivep, 1, 0),
	FUNCTION("negative?", scm_negativep, 1, 0),
	FUNCTION("odd?",      scm_oddp,      1, 0),
	FUNCTION("even?",     scm_evenp,     1, 0),

	FUNCTION("number->string", scm_number_to_string, 1, 1),
	FUNCTION("string->number", scm_string_to_number, 1, 1),

	FUNCTION("not",       scm_not,        1, 0),
	FUNCTION("boolean?",  scm_booleanp,   1, 0),
	FUNCTION("boolean=?", scm_boolean_eq, 1, 1),

	FUNCTION("cons",   scm_cons,   2, 0),
	FUNCTION("car",    scm_car,    1, 0),
	FUNCTION("cdr",    scm_cdr,    1, 0),
	FUNCTION("pair?",  scm_pairp,  1, 0),
	FUNCTION("list?",  scm_listp,  1, 0),
	FUNCTION("null?",  scm_nullp,  1, 0),
	FUNCTION("list",   scm_list,   0, 1),
	FUNCTION("length", scm_length, 1, 0),

	FUNCTION("char?",      scm_charp,    1, 0),
	FUNCTION("char<?",     scm_char_lt,  2, 0),
	FUNCTION("char>?",     scm_char_gt,  2, 0),
	FUNCTION("char=?",     scm_char_eq,  2, 0),
	FUNCTION("char<=?",    scm_char_lte, 2, 0),
	FUNCTION("char>=>",    scm_char_gte, 2, 0),
	FUNCTION("char-ci<?",  scm_char_ci_lt,  2, 0),
	FUNCTION("char-ci>?",  scm_char_ci_gt,  2, 0),
	FUNCTION("char-ci=?",  scm_char_ci_eq,  2, 0),
	FUNCTION("char-ci<=?", scm_char_ci_lte, 2, 0),
	FUNCTION("char-ci>=>", scm_char_ci_gte, 2, 0),

	FUNCTION("char-upcase",   scm_char_upcase,   1, 0),
	FUNCTION("char-downcase", scm_char_downcase, 1, 0),

	FUNCTION("string?",       scm_stringp,        1, 0),
	FUNCTION("make-string",   scm_make_string,    1, 1),
	FUNCTION("string",        scm_string,         0, 1),
	FUNCTION("string-length", scm_string_length,  1, 0),
	FUNCTION("string-ref",    scm_string_ref,     2, 0),
	FUNCTION("string-set!",   scm_string_set,     3, 0),
	FUNCTION("string<?",      scm_string_lt,      2, 0),
	FUNCTION("string>?",      scm_string_gt,      2, 0),
	FUNCTION("string=?",      scm_string_eq,      2, 0),
	FUNCTION("string<=?",     scm_string_lte,     2, 0),
	FUNCTION("string>=?",     scm_string_gte,     2, 0),
	FUNCTION("string-ci<?",   scm_string_ci_lt,   2, 0),
	FUNCTION("string-ci>?",   scm_string_ci_gt,   2, 0),
	FUNCTION("string-ci=?",   scm_string_ci_eq,   2, 0),
	FUNCTION("string-ci<=?",  scm_string_ci_lte,  2, 0),
	FUNCTION("string-ci>=?",  scm_string_ci_gte,  2, 0),
	FUNCTION("substring",     scm_substring,      3, 0),
	FUNCTION("string-append", scm_string_append,  1, 1),
	FUNCTION("string->list",  scm_string_to_list, 1, 0),
	FUNCTION("list->string",  scm_list_to_string, 1, 0),
	FUNCTION("string-fill!",  scm_string_fill,    2, 1),
	FUNCTION("string-copy",   scm_string_copy,    1, 1),
	FUNCTION("string-copy!",  scm_string_copy_to, 3, 1),

	FUNCTION("vector?",       scm_vectorp,        1, 0),
	FUNCTION("make-vector",   scm_make_vector,    1, 1),
	FUNCTION("vector",        scm_vector,         0, 1),
	FUNCTION("vector-length", scm_vector_length,  1, 0),
	FUNCTION("vector-ref",    scm_vector_ref,     2, 0),
	FUNCTION("vector-set!",   scm_vector_set,     3, 0),
	FUNCTION("vector-fill!",  scm_vector_fill,    2, 1),
	FUNCTION("vector-copy",   scm_vector_copy,    1, 1),
	FUNCTION("vector-copy!",  scm_vector_copy_to, 3, 1),

	FUNCTION("bytevector?",        scm_bytevectorp,        1, 0),
	FUNCTION("make-bytevector",    scm_make_bytevector,    1, 1),
	FUNCTION("bytevector",         scm_bytevector,         0, 1),
	FUNCTION("bytevector-length",  scm_bytevector_length,  1, 0),
	FUNCTION("bytevector-u8-ref",  scm_bytevector_u8_ref,  2, 0),
	FUNCTION("bytevector-u8-set!", scm_bytevector_u8_set,  3, 0),
	FUNCTION("bytevector-append",  scm_bytevector_append,  0, 1),
	FUNCTION("bytevector-copy",    scm_bytevector_copy,    1, 1),
	FUNCTION("bytevector-copy!",   scm_bytevector_copy_to, 3, 1),
	FUNCTION("utf8->string",       scm_utf8_to_string,     1, 1),
	FUNCTION("string->utf8",       scm_string_to_utf8,     1, 1),

	FUNCTION("env-count",     scm_env_count,     0, 0),
	FUNCTION("gc-collect",    scm_gc_collect,    0, 0),
	FUNCTION("gc-count",      scm_gc_count,      0, 0),
};
#define NR_DEFAULT_BINDINGS \
	(sizeof(default_bindings) / sizeof(*default_bindings))

/*
 * The top-level exception handler: prints a message and returns to the REPL.
 */
static DEFUN(scm_toplevel_exn, args, env)
{
	navi_t cont;

	navi_write(navi_car(args), env);
	putchar('\n');

	cont = navi_env_lookup(env, navi_sym_repl);
	if (navi_type(cont) != NAVI_ESCAPE)
		navi_die("#repl not bound to continuation");

	navi_scope_set(env, navi_sym_exn, navi_from_spec(&default_bindings[0]));
	longjmp(navi_escape(cont)->state, 1);
}


