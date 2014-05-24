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

#include "sexp.h"

static sexp_t call_read(env_t env)
{
	return scm_read(make_nil(), env);
}

static _Noreturn void repl(void)
{
	env_t env = make_default_environment();
	sexp_t cont = make_escape();
	struct sexp_escape *escape = sexp_escape(cont);

	scope_set(env, sym_repl, cont);

	for (volatile int i = 0;; i++) {
		sexp_t sexp;

		setjmp(escape->state);

		printf("\n#;%d> ", i);
		if ((sexp = call_read(env)).n == 0)
			continue;
		printf("read: "); sexp_write(sexp); putchar('\n');
		if (is_eof(sexp))
			break;
		if ((sexp = trampoline(sexp, env)).n == 0)
			continue;
		sexp_write(sexp);
	}
	putchar('\n');
	exit(0);
}

static _Noreturn void script(void)
{
	env_t env = make_default_environment();

	for (;;) {
		sexp_t sexp = call_read(env);
		if (is_eof(sexp))
			break;
		display(trampoline(sexp, env));
	}
	putchar('\n');
	exit(0);
}

int main(int argc, char *argv[])
{
	symbol_table_init();
	if (argc == 1)
		repl();
	else
		script();
}
