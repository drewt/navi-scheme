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

#ifndef _NAVI_LIB_H
#define _NAVI_LIB_H

#include "types.h"
#include "macros.h"

DECLARE(lambda);
DECLARE(caselambda);
DECLARE(define);
DECLARE(define_values);
DECLARE(defmacro);
DECLARE(begin);
DECLARE(let);
DECLARE(sequential_let);
DECLARE(let_values);
DECLARE(set);
DECLARE(quote);
DECLARE(unquote);
DECLARE(quasiquote);
DECLARE(case);
DECLARE(cond);
DECLARE(if);
DECLARE(and);
DECLARE(or);
DECLARE(delay);

DECLARE(gensym);

DECLARE(eval);
DECLARE(apply);
DECLARE(force);
DECLARE(promisep);
DECLARE(make_promise);

DECLARE(call_ec);
DECLARE(values);
DECLARE(call_with_values);
DECLARE(with_exception_handler);
DECLARE(raise);
DECLARE(raise_continuable);
DECLARE(error);
DECLARE(error_objectp);
DECLARE(error_object_message);
DECLARE(error_object_irritants);
DECLARE(read_errorp);

DECLARE(input_portp);
DECLARE(output_portp);
DECLARE(portp);
DECLARE(input_port_openp);
DECLARE(output_port_openp);
DECLARE(current_input_port);
DECLARE(current_output_port);
DECLARE(current_error_port);
DECLARE(open_input_file);
DECLARE(open_output_file);
DECLARE(open_input_string);
DECLARE(open_output_string);
DECLARE(get_output_string);
DECLARE(close_port);
DECLARE(close_input_port);
DECLARE(close_output_port);
DECLARE(eof_objectp);
DECLARE(eof_object);
DECLARE(read_u8);
DECLARE(peek_u8);
DECLARE(read_char);
DECLARE(peek_char);
DECLARE(read);
DECLARE(write_u8);
DECLARE(write_char);
DECLARE(write_string);
DECLARE(write);
DECLARE(display);
DECLARE(newline);

DECLARE(eqvp);

DECLARE(add);
DECLARE(sub);
DECLARE(mul);
DECLARE(div);
DECLARE(quotient);
DECLARE(remainder);

DECLARE(lt);
DECLARE(gt);
DECLARE(lte);
DECLARE(gte);
DECLARE(numeq);
DECLARE(zerop);
DECLARE(positivep);
DECLARE(negativep);
DECLARE(oddp);
DECLARE(evenp);

DECLARE(number_to_string);
DECLARE(string_to_number);

DECLARE(not);
DECLARE(booleanp);
DECLARE(boolean_eq);

DECLARE(cons);
DECLARE(car);
DECLARE(cdr);
DECLARE(pairp);
DECLARE(listp);
DECLARE(nullp);
DECLARE(list);
DECLARE(length);
DECLARE(map);

DECLARE(charp);
DECLARE(char_lt);
DECLARE(char_gt);
DECLARE(char_eq);
DECLARE(char_lte);
DECLARE(char_gte);
DECLARE(char_ci_lt);
DECLARE(char_ci_gt);
DECLARE(char_ci_eq);
DECLARE(char_ci_lte);
DECLARE(char_ci_gte);
DECLARE(char_upcase);
DECLARE(char_downcase);

DECLARE(stringp);
DECLARE(make_string);
DECLARE(string_length);
DECLARE(string);
DECLARE(string_ref);
DECLARE(string_set);
DECLARE(string_lt);
DECLARE(string_gt);
DECLARE(string_eq);
DECLARE(string_lte);
DECLARE(string_gte);
DECLARE(string_ci_lt);
DECLARE(string_ci_gt);
DECLARE(string_ci_eq);
DECLARE(string_ci_lte);
DECLARE(string_ci_gte);
DECLARE(string_upcase);
DECLARE(string_downcase);
DECLARE(string_foldcase);
DECLARE(substring);
DECLARE(string_append);
DECLARE(string_to_list);
DECLARE(list_to_string);
DECLARE(string_fill);
DECLARE(string_copy);
DECLARE(string_copy_to);
DECLARE(string_map);
DECLARE(string_map_ip);

DECLARE(vectorp);
DECLARE(make_vector);
DECLARE(vector);
DECLARE(vector_length);
DECLARE(vector_ref);
DECLARE(vector_set);
DECLARE(vector_to_list);
DECLARE(list_to_vector);
DECLARE(vector_fill);
DECLARE(vector_copy);
DECLARE(vector_copy_to);
DECLARE(vector_map);
DECLARE(vector_map_ip);

DECLARE(bytevectorp);
DECLARE(make_bytevector);
DECLARE(bytevector);
DECLARE(bytevector_length);
DECLARE(bytevector_u8_ref);
DECLARE(bytevector_u8_set);
DECLARE(bytevector_append);
DECLARE(bytevector_copy);
DECLARE(bytevector_copy_to);
DECLARE(utf8_to_string);
DECLARE(string_to_utf8);

DECLARE(env_count);
DECLARE(gc_collect);
DECLARE(gc_count);

#endif
