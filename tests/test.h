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

#ifndef _TESTS_TEST_H
#define _TESTS_TEST_H

#include <stdlib.h>
#include <check.h>
#include "../sexp.h"

#define CALL0(fn) fn(make_nil(), env)
#define CALL(fn, ...) fn(list(__VA_ARGS__, make_void()), env)

env_t env;

Suite *arithmetic_suite(void);

#endif