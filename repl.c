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

#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <setjmp.h>

#include "navi.h"

static navi_t call_read(navi_env_t env)
{
	return scm_read(navi_make_nil(), env);
}

static _Noreturn void repl(void)
{
	navi_env_t env = navi_make_default_environment();
	navi_t cont = navi_make_escape();
	struct navi_escape *escape = navi_escape(cont);

	navi_scope_set(env, navi_sym_repl, cont);

	for (volatile int i = 0;; i++) {
		navi_t expr;

		setjmp(escape->state);

		printf("\n#;%d> ", i);
		if ((expr = call_read(env)).n == 0)
			continue;
		printf("read: "); navi_write(expr, env); putchar('\n');
		if (navi_is_eof(expr))
			break;
		if ((expr = navi_eval(expr, env)).n == 0)
			continue;
		navi_write(expr, env);
	}
	putchar('\n');
	exit(0);
}

static _Noreturn void script(void)
{
	navi_env_t env = navi_make_default_environment();

	for (;;) {
		navi_t expr = call_read(env);
		if (navi_is_eof(expr))
			break;
		navi_write(navi_eval(expr, env), env);
	}
	putchar('\n');
	exit(0);
}

int main(int argc, char *argv[])
{
	navi_init();
	if (argc == 1)
		repl();
	else
		script();
}
