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

#ifndef _SEXP_SYMBOLS_H
#define _SEXP_SYMBOLS_H

/* automatically interned symbols */
extern sexp_t sym_lambda;
extern sexp_t sym_define;
extern sexp_t sym_begin;
extern sexp_t sym_let;
extern sexp_t sym_seqlet;
extern sexp_t sym_letrec;
extern sexp_t sym_seqletrec;
extern sexp_t sym_set;
extern sexp_t sym_quote;
extern sexp_t sym_quasiquote;
extern sexp_t sym_unquote;
extern sexp_t sym_splice;
extern sexp_t sym_case;
extern sexp_t sym_cond;
extern sexp_t sym_if;
extern sexp_t sym_and;
extern sexp_t sym_or;
extern sexp_t sym_else;
extern sexp_t sym_eq_lt;
extern sexp_t sym_question;
extern sexp_t sym_exn;
extern sexp_t sym_repl;

#endif
