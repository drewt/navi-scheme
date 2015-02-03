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

#ifndef _NAVI_SYMBOLS_H
#define _NAVI_SYMBOLS_H

/* automatically interned symbols */
extern navi_t navi_sym_lambda;
extern navi_t navi_sym_caselambda;
extern navi_t navi_sym_define;
extern navi_t navi_sym_defmacro;
extern navi_t navi_sym_begin;
extern navi_t navi_sym_let;
extern navi_t navi_sym_seqlet;
extern navi_t navi_sym_letrec;
extern navi_t navi_sym_seqletrec;
extern navi_t navi_sym_set;
extern navi_t navi_sym_quote;
extern navi_t navi_sym_quasiquote;
extern navi_t navi_sym_unquote;
extern navi_t navi_sym_splice;
extern navi_t navi_sym_case;
extern navi_t navi_sym_cond;
extern navi_t navi_sym_if;
extern navi_t navi_sym_and;
extern navi_t navi_sym_or;
extern navi_t navi_sym_else;
extern navi_t navi_sym_eq_lt;
extern navi_t navi_sym_question;
extern navi_t navi_sym_exn;
extern navi_t navi_sym_current_input;
extern navi_t navi_sym_current_output;
extern navi_t navi_sym_current_error;
extern navi_t navi_sym_read_error;
extern navi_t navi_sym_file_error;
extern navi_t navi_sym_repl;

#endif
