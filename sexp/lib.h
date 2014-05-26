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

#ifndef _SEXP_LIB_H
#define _SEXP_LIB_H

#include "types.h"

#define DECLARE(fname) sexp_t fname (sexp_t,env_t)

DECLARE(eval_lambda);
DECLARE(eval_caselambda);
DECLARE(eval_define);
DECLARE(eval_defmacro);
DECLARE(eval_begin);
DECLARE(eval_let);
DECLARE(eval_sequential_let);
DECLARE(eval_set);
DECLARE(eval_quote);
DECLARE(eval_quasiquote);
DECLARE(eval_case);
DECLARE(eval_cond);
DECLARE(eval_if);
DECLARE(eval_and);
DECLARE(eval_or);

DECLARE(scm_gensym);

DECLARE(scm_read);
DECLARE(scm_eval);
DECLARE(scm_apply);
DECLARE(scm_map);
DECLARE(scm_string_map);
DECLARE(scm_string_map_ip);

DECLARE(scm_call_ec);
DECLARE(scm_values);
DECLARE(scm_call_with_values);
DECLARE(scm_with_exception_handler);
_Noreturn DECLARE(scm_raise);
DECLARE(scm_raise_continuable);
DECLARE(scm_error);
DECLARE(scm_error_objectp);
DECLARE(scm_error_object_message);
DECLARE(scm_error_object_irritants);
DECLARE(scm_read_errorp);

DECLARE(scm_current_input_port);
DECLARE(scm_current_output_port);
DECLARE(scm_current_error_port);
DECLARE(scm_open_input_string);
DECLARE(scm_eof_objectp);
DECLARE(scm_eof_object);
DECLARE(scm_read_u8);
DECLARE(scm_peek_u8);
DECLARE(scm_read_char);
DECLARE(scm_peek_char);
DECLARE(scm_write_u8);
DECLARE(scm_write_char);
DECLARE(scm_write_string);
DECLARE(scm_write);
DECLARE(scm_display);
DECLARE(scm_newline);

DECLARE(scm_eqvp);

DECLARE(scm_add);
DECLARE(scm_sub);
DECLARE(scm_mul);
DECLARE(scm_div);
DECLARE(scm_quotient);
DECLARE(scm_remainder);

DECLARE(scm_lt);
DECLARE(scm_gt);
DECLARE(scm_lte);
DECLARE(scm_gte);
DECLARE(scm_numeq);
DECLARE(scm_zerop);
DECLARE(scm_positivep);
DECLARE(scm_negativep);
DECLARE(scm_oddp);
DECLARE(scm_evenp);

DECLARE(scm_number_to_string);
DECLARE(scm_string_to_number);

DECLARE(scm_not);
DECLARE(scm_booleanp);
DECLARE(scm_boolean_eq);

DECLARE(scm_cons);
DECLARE(scm_car);
DECLARE(scm_cdr);
DECLARE(scm_pairp);
DECLARE(scm_listp);
DECLARE(scm_nullp);
DECLARE(scm_list);
DECLARE(scm_length);

DECLARE(scm_charp);
DECLARE(scm_char_lt);
DECLARE(scm_char_gt);
DECLARE(scm_char_eq);
DECLARE(scm_char_lte);
DECLARE(scm_char_gte);
DECLARE(scm_char_ci_lt);
DECLARE(scm_char_ci_gt);
DECLARE(scm_char_ci_eq);
DECLARE(scm_char_ci_lte);
DECLARE(scm_char_ci_gte);
DECLARE(scm_char_upcase);
DECLARE(scm_char_downcase);

DECLARE(scm_stringp);
DECLARE(scm_make_string);
DECLARE(scm_string_length);
DECLARE(scm_string);
DECLARE(scm_string_ref);
DECLARE(scm_string_set);
DECLARE(scm_string_lt);
DECLARE(scm_string_gt);
DECLARE(scm_string_eq);
DECLARE(scm_string_lte);
DECLARE(scm_string_gte);
DECLARE(scm_string_ci_lt);
DECLARE(scm_string_ci_gt);
DECLARE(scm_string_ci_eq);
DECLARE(scm_string_ci_lte);
DECLARE(scm_string_ci_gte);
DECLARE(scm_substring);
DECLARE(scm_string_append);
DECLARE(scm_string_to_list);
DECLARE(scm_list_to_string);
DECLARE(scm_string_fill);
DECLARE(scm_string_copy);
DECLARE(scm_string_copy_to);

DECLARE(scm_vectorp);
DECLARE(scm_make_vector);
DECLARE(scm_vector);
DECLARE(scm_vector_length);
DECLARE(scm_vector_ref);
DECLARE(scm_vector_set);
DECLARE(scm_vector_to_list);
DECLARE(scm_list_to_vector);
DECLARE(scm_vector_fill);

DECLARE(scm_env_count);
DECLARE(scm_gc_collect);
DECLARE(scm_gc_count);

#endif
