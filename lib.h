/* Copyright 2014-2015 Drew Thoreson
 *
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/.
 */

#ifndef _NAVI_LIB_H
#define _NAVI_LIB_H

#include "internal.h"
#include "macros.h"

DECLARE(lambda);
DECLARE(caselambda);
DECLARE(define);
DECLARE(define_syntax);
DECLARE(define_values);
DECLARE(defmacro);
DECLARE(define_library);
DECLARE(import);
DECLARE(begin);
DECLARE(include);
DECLARE(include_ci);
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
DECLARE(make_parameter);
DECLARE(parameterize);

DECLARE(command_line);
DECLARE(emergency_exit);
DECLARE(exit);
DECLARE(get_environment_variable);
DECLARE(get_environment_variables);
DECLARE(current_second);
DECLARE(current_jiffy);
DECLARE(jiffies_per_second);

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
DECLARE(eqp);
DECLARE(equalp);

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
DECLARE(set_car);
DECLARE(set_cdr);
DECLARE(pairp);
DECLARE(listp);
DECLARE(nullp);
DECLARE(list);
DECLARE(make_list);
DECLARE(length);
DECLARE(append);
DECLARE(reverse);
DECLARE(list_tail);
DECLARE(list_ref);
DECLARE(list_set);
DECLARE(memq);
DECLARE(memv);
DECLARE(member);
DECLARE(assq);
DECLARE(assv);
DECLARE(assoc);
DECLARE(list_copy);
DECLARE(for_each);
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
DECLARE(string_to_vector);
DECLARE(vector_to_string);
DECLARE(string_fill);
DECLARE(string_copy);
DECLARE(string_copy_to);

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
DECLARE(vector_for_each);
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

DECLARE(env_list);
DECLARE(env_count);
DECLARE(env_show);
DECLARE(gc_collect);
DECLARE(gc_count);
DECLARE(gc_stats);

#endif
