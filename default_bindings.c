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

/* PROC_TYPE(const char *scmname, navi_builtin_t _c_proc, unsigned _arity, uint32_t flags)
 * @scmname:	the (Scheme) name of the procedure
 * @_c_proc:	the (C) procedure to which the object refers
 * @_arity:	the (minimum) arity of the procedure
 * @_flags:	bitwise OR of zero or more NAVI_PROC_* flags
 * @_type:	the type of the created object (procedure, macro, etc.)
 */
#define PROC_TYPE(scmname, _c_proc, _arity, _flags, _type) \
{ \
	.type = _type, \
	.proc = { \
		.c_proc   = _c_proc, \
		.arity    = _arity, \
		.flags    = NAVI_PROC_BUILTIN | _flags, \
	}, \
	.ident = scmname, \
}

#define SPECIAL(scmname, _fn, ary, var) \
	PROC_TYPE(scmname, _fn, ary, var, NAVI_SPECIAL)

#define PROCEDURE(scmname, _fn, ary, var) \
	PROC_TYPE(scmname, _fn, ary, var, NAVI_PROCEDURE)

#define MACRO(scmname, _fn, ary, var) \
	PROC_TYPE(scmname, _fn, ary, var, NAVI_MACRO)

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

#define V NAVI_PROC_VARIADIC
static struct navi_spec default_bindings[] = {
	[TOPLEVEL_EXN_INDEX] = PROCEDURE("#exn",  scm_toplevel_exn, 1, 0),

	SPECIAL("lambda",        scm_lambda,         2, V),
	SPECIAL("case-lambda",   scm_caselambda,     2, V),
	SPECIAL("define",        scm_define,         2, V),
	SPECIAL("define-values", scm_define_values,  2, V),
	SPECIAL("define-macro",  scm_defmacro,       2, V),
	SPECIAL("begin",         scm_begin,          1, V),
	SPECIAL("let",           scm_let,            2, V),
	SPECIAL("let*",          scm_sequential_let, 2, V),
	SPECIAL("letrec",        scm_let,            2, V),
	SPECIAL("letrec*",       scm_sequential_let, 2, V),
	SPECIAL("let-values",    scm_let_values,     2, V),
	SPECIAL("set!",          scm_set,            2, 0),
	SPECIAL("quote",         scm_quote,          1, 0),
	SPECIAL("quasiquote",    scm_quasiquote,     1, 0),
	SPECIAL("case",          scm_case,           2, V),
	SPECIAL("cond",          scm_cond,           1, V),
	SPECIAL("if",            scm_if,             2, V),
	SPECIAL("and",           scm_and,            0, V),
	SPECIAL("or",            scm_or,             0, V),
	SPECIAL("delay",         scm_delay,          1, 0),

	PROCEDURE("gensym", scm_gensym, 0, 0),

	PROCEDURE("eval",        scm_eval,          1, 0),
	PROCEDURE("apply",       scm_apply,         2, V),
	PROCEDURE("map",         scm_map,           2, 0),
	PROCEDURE("string-map",  scm_string_map,    2, 0),
	PROCEDURE("string-map!", scm_string_map_ip, 2, 0),
	PROCEDURE("vector-map",  scm_vector_map,    2, 0),
	PROCEDURE("vector-map!", scm_vector_map_ip, 2, 0),

	PROCEDURE("force",        scm_force,        1, 0),
	PROCEDURE("promise?",     scm_promisep,     1, 0),
	PROCEDURE("make-promise", scm_make_promise, 1, 0),

	PROCEDURE("call/ec", scm_call_ec, 0, V),
	PROCEDURE("values", scm_values, 1, V),
	PROCEDURE("call-with-values", scm_call_with_values, 2, 0),
	PROCEDURE("with-exception-handler", scm_with_exception_handler, 2, 0),
	PROCEDURE("raise", scm_raise, 1, 0),
	PROCEDURE("raise-continuable", scm_raise_continuable, 1, 0),
	PROCEDURE("error", scm_error, 1, V),
	PROCEDURE("error-object?", scm_error_objectp, 1, 0),
	PROCEDURE("error-object-message", scm_error_object_message, 1, 0),
	PROCEDURE("error-object-irritants", scm_error_object_irritants, 1, 0),
	PROCEDURE("read-error?", scm_read_errorp, 1, 0),

	PROCEDURE("input-port?",             scm_input_portp,         1, 0),
	PROCEDURE("output-port?",            scm_output_portp,        1, 0),
	PROCEDURE("textual-port?",           scm_portp,               1, 0),
	PROCEDURE("binary-port?",            scm_portp,               1, 0),
	PROCEDURE("port?",                   scm_portp,               1, 0),
	PROCEDURE("input-port-open?",        scm_input_port_openp,    1, 0),
	PROCEDURE("output-port-open?",       scm_output_port_openp,   1, 0),
	PROCEDURE("current-input-port",      scm_current_input_port,  0, 0),
	PROCEDURE("current-output-port",     scm_current_output_port, 0, 0),
	PROCEDURE("current-error-port",      scm_current_error_port,  0, 0),
	PROCEDURE("open-input-file",         scm_open_input_file,     1, 0),
	PROCEDURE("open-binary-input-file",  scm_open_input_file,     1, 0),
	PROCEDURE("open-output-file",        scm_open_output_file,    1, 0),
	PROCEDURE("open-binary-output-file", scm_open_output_file,    1, 0),
	PROCEDURE("open-input-string",       scm_open_input_string,   1, 0),
	PROCEDURE("open-output-string",      scm_open_output_string,  0, 0),
	PROCEDURE("get-output-string",       scm_get_output_string,   1, 0),
	PROCEDURE("close-port",              scm_close_port,          1, 0),
	PROCEDURE("close-input-port",        scm_close_input_port,    1, 0),
	PROCEDURE("close-output-port",       scm_close_output_port,   1, 0),
	PROCEDURE("eof-object?",             scm_eof_objectp,         1, 0),
	PROCEDURE("eof-object",              scm_eof_object,          0, 0),
	PROCEDURE("read-u8",                 scm_read_u8,             0, V),
	PROCEDURE("peek-u8",                 scm_peek_u8,             0, V),
	PROCEDURE("read-char",               scm_read_char,           0, V),
	PROCEDURE("peek-char",               scm_peek_char,           0, V),
	PROCEDURE("read",                    scm_read,                0, V),
	PROCEDURE("write-u8",                scm_write_u8,            1, V),
	PROCEDURE("write-char",              scm_write_char,          1, V),
	PROCEDURE("write-string",            scm_write_string,        1, V),
	PROCEDURE("write",                   scm_write,               1, V),
	PROCEDURE("display",                 scm_display,             1, V),
	PROCEDURE("newline",                 scm_newline,             0, V),

	PROCEDURE("eqv?", scm_eqvp, 2, 0),

	PROCEDURE("+", scm_add, 0, V),
	PROCEDURE("-", scm_sub, 1, V),
	PROCEDURE("*", scm_mul, 0, V),
	PROCEDURE("/", scm_div, 1, V),
	PROCEDURE("quotient",  scm_quotient,  2, 0),
	PROCEDURE("remainder", scm_remainder, 2, 0),

	PROCEDURE("<",  scm_lt,    1, V),
	PROCEDURE(">",  scm_gt,    1, V),
	PROCEDURE("<=", scm_lte,   1, V),
	PROCEDURE(">=", scm_gte,   1, V),
	PROCEDURE("=",  scm_numeq, 1, V),

	PROCEDURE("zero?",     scm_zerop,     1, 0),
	PROCEDURE("positive?", scm_positivep, 1, 0),
	PROCEDURE("negative?", scm_negativep, 1, 0),
	PROCEDURE("odd?",      scm_oddp,      1, 0),
	PROCEDURE("even?",     scm_evenp,     1, 0),

	PROCEDURE("number->string", scm_number_to_string, 1, V),
	PROCEDURE("string->number", scm_string_to_number, 1, V),

	PROCEDURE("not",       scm_not,        1, 0),
	PROCEDURE("boolean?",  scm_booleanp,   1, 0),
	PROCEDURE("boolean=?", scm_boolean_eq, 1, V),

	PROCEDURE("cons",   scm_cons,   2, 0),
	PROCEDURE("car",    scm_car,    1, 0),
	PROCEDURE("cdr",    scm_cdr,    1, 0),
	PROCEDURE("pair?",  scm_pairp,  1, 0),
	PROCEDURE("list?",  scm_listp,  1, 0),
	PROCEDURE("null?",  scm_nullp,  1, 0),
	PROCEDURE("list",   scm_list,   0, V),
	PROCEDURE("length", scm_length, 1, 0),

	PROCEDURE("char?",      scm_charp,    1, 0),
	PROCEDURE("char<?",     scm_char_lt,  2, 0),
	PROCEDURE("char>?",     scm_char_gt,  2, 0),
	PROCEDURE("char=?",     scm_char_eq,  2, 0),
	PROCEDURE("char<=?",    scm_char_lte, 2, 0),
	PROCEDURE("char>=>",    scm_char_gte, 2, 0),
	PROCEDURE("char-ci<?",  scm_char_ci_lt,  2, 0),
	PROCEDURE("char-ci>?",  scm_char_ci_gt,  2, 0),
	PROCEDURE("char-ci=?",  scm_char_ci_eq,  2, 0),
	PROCEDURE("char-ci<=?", scm_char_ci_lte, 2, 0),
	PROCEDURE("char-ci>=>", scm_char_ci_gte, 2, 0),

	PROCEDURE("char-upcase",   scm_char_upcase,   1, 0),
	PROCEDURE("char-downcase", scm_char_downcase, 1, 0),

	PROCEDURE("string?",         scm_stringp,         1, 0),
	PROCEDURE("make-string",     scm_make_string,     1, V),
	PROCEDURE("string",          scm_string,          0, V),
	PROCEDURE("string-length",   scm_string_length,   1, 0),
	PROCEDURE("string-ref",      scm_string_ref,      2, 0),
	PROCEDURE("string-set!",     scm_string_set,      3, 0),
	PROCEDURE("string<?",        scm_string_lt,       2, 0),
	PROCEDURE("string>?",        scm_string_gt,       2, 0),
	PROCEDURE("string=?",        scm_string_eq,       2, 0),
	PROCEDURE("string<=?",       scm_string_lte,      2, 0),
	PROCEDURE("string>=?",       scm_string_gte,      2, 0),
	PROCEDURE("string-ci<?",     scm_string_ci_lt,    2, 0),
	PROCEDURE("string-ci>?",     scm_string_ci_gt,    2, 0),
	PROCEDURE("string-ci=?",     scm_string_ci_eq,    2, 0),
	PROCEDURE("string-ci<=?",    scm_string_ci_lte,   2, 0),
	PROCEDURE("string-ci>=?",    scm_string_ci_gte,   2, 0),
	PROCEDURE("string-upcase",   scm_string_upcase,   1, 0),
	PROCEDURE("string-downcase", scm_string_downcase, 1, 0),
	PROCEDURE("string-foldcase", scm_string_foldcase, 1, 0),
	PROCEDURE("substring",       scm_substring,       3, 0),
	PROCEDURE("string-append",   scm_string_append,   1, V),
	PROCEDURE("string->list",    scm_string_to_list,  1, 0),
	PROCEDURE("list->string",    scm_list_to_string,  1, 0),
	PROCEDURE("string-fill!",    scm_string_fill,     2, V),
	PROCEDURE("string-copy",     scm_string_copy,     1, V),
	PROCEDURE("string-copy!",    scm_string_copy_to,  3, V),

	PROCEDURE("vector?",       scm_vectorp,        1, 0),
	PROCEDURE("make-vector",   scm_make_vector,    1, V),
	PROCEDURE("vector",        scm_vector,         0, V),
	PROCEDURE("vector-length", scm_vector_length,  1, 0),
	PROCEDURE("vector-ref",    scm_vector_ref,     2, 0),
	PROCEDURE("vector-set!",   scm_vector_set,     3, 0),
	PROCEDURE("vector-fill!",  scm_vector_fill,    2, V),
	PROCEDURE("vector-copy",   scm_vector_copy,    1, V),
	PROCEDURE("vector-copy!",  scm_vector_copy_to, 3, V),

	PROCEDURE("bytevector?",        scm_bytevectorp,        1, 0),
	PROCEDURE("make-bytevector",    scm_make_bytevector,    1, V),
	PROCEDURE("bytevector",         scm_bytevector,         0, V),
	PROCEDURE("bytevector-length",  scm_bytevector_length,  1, 0),
	PROCEDURE("bytevector-u8-ref",  scm_bytevector_u8_ref,  2, 0),
	PROCEDURE("bytevector-u8-set!", scm_bytevector_u8_set,  3, 0),
	PROCEDURE("bytevector-append",  scm_bytevector_append,  0, V),
	PROCEDURE("bytevector-copy",    scm_bytevector_copy,    1, V),
	PROCEDURE("bytevector-copy!",   scm_bytevector_copy_to, 3, V),
	PROCEDURE("utf8->string",       scm_utf8_to_string,     1, V),
	PROCEDURE("string->utf8",       scm_string_to_utf8,     1, V),

	PROCEDURE("env-count",     scm_env_count,     0, 0),
	PROCEDURE("gc-collect",    scm_gc_collect,    0, 0),
	PROCEDURE("gc-count",      scm_gc_count,      0, 0),
};
#undef V
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


