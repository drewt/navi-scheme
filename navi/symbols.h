/* Copyright 2014-2015 Drew Thoreson
 *
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/.
 */

#ifndef _NAVI_SYMBOLS_H
#define _NAVI_SYMBOLS_H

/* automatically interned symbols */
extern navi_obj navi_sym_begin;
extern navi_obj navi_sym_quote;
extern navi_obj navi_sym_quasiquote;
extern navi_obj navi_sym_unquote;
extern navi_obj navi_sym_splice;
extern navi_obj navi_sym_else;
extern navi_obj navi_sym_eq_lt;
extern navi_obj navi_sym_lib_paths;
extern navi_obj navi_sym_command_line;
extern navi_obj navi_sym_current_exn;
extern navi_obj navi_sym_current_input;
extern navi_obj navi_sym_current_output;
extern navi_obj navi_sym_current_error;
extern navi_obj navi_sym_read_error;
extern navi_obj navi_sym_file_error;
extern navi_obj navi_sym_internal_error;
extern navi_obj navi_sym_repl;
extern navi_obj navi_sym_export;
extern navi_obj navi_sym_import;
extern navi_obj navi_sym_only;
extern navi_obj navi_sym_except;
extern navi_obj navi_sym_prefix;
extern navi_obj navi_sym_rename;
extern navi_obj navi_sym_deflib;
extern navi_obj navi_sym_include;
extern navi_obj navi_sym_include_ci;
extern navi_obj navi_sym_include_libdecl;
extern navi_obj navi_sym_cond_expand;
extern navi_obj navi_sym_ellipsis;
extern navi_obj navi_sym_underscore;

#endif
