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

#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <setjmp.h>
#include <getopt.h>

#include "navi.h"

static _Noreturn void usage(const char *name, int status)
{
	printf("usage: %s [OPTION ...] [FILENAME [ARGUMENT ...]]\n", name);
	puts("  FILENAME is a Scheme source file, or '-' to read from standard input.");
	puts("  OPTION may be one of the following:\n");
	puts("    -h, --help     display this text and exit");
	puts("        --version  display version and exit");
	exit(status);
}

static void version(void)
{
	puts("NAVI Scheme " NAVI_VERSION);
	puts("(c) 2014-2015 Drew Thoreson");
}

static struct option long_options[] = {
	{ "help",    no_argument, 0, 'h' },
	{ "version", no_argument, 0, 'V' },
};

struct navi_options {
	char *filename;
	char **argv;
};

void parse_opts(int argc, char *argv[], struct navi_options *options)
{
	// separate navii options from program arguments
	for (int i = 1; i < argc; i++) {
		if (argv[i][0] != '-' || !strcmp(argv[i], "-")) {
			options->filename = argv[i];
			options->argv = &argv[i+1];
			argv[i] = NULL;
			argc = i;
			break;
		}
	}

	// parse navii options
	for (;;) {
		int options_index = 0;
		int c = getopt_long(argc, argv, "h", long_options, &options_index);
		if (c < 0)
			break;
		switch (c) {
		case 'h':
			usage(argv[0], EXIT_SUCCESS);
		case 'V':
			version();
			exit(EXIT_SUCCESS);
		case '?':
			break;
		default:
			usage(argv[0], EXIT_FAILURE);
		}
	}
}

static navi_obj call_read(navi_env env)
{
	return navi_read(navi_port(navi_current_input_port(env)), env);
}

static _Noreturn void repl(struct navi_options *options)
{
	navi_env env = navi_interaction_environment();
	navi_obj cont = navi_make_escape();
	struct navi_escape *escape = navi_escape(cont);

	navi_scope_set(env.lexical, navi_sym_repl, cont);

	for (volatile int i = 0;; i++) {
		navi_obj expr;

		if (setjmp(escape->state))
			i++;

		printf("#;%d> ", i);
		if ((expr = call_read(env)).n == 0)
			continue;
		if (navi_is_eof(expr))
			break;
		if ((expr = navi_eval(expr, env)).n == 0)
			continue;
		navi_write(expr, env);
		if (!navi_is_void(expr))
			putchar('\n');
	}
	putchar('\n');
	exit(0);
}

static _Noreturn void script(void)
{
	navi_env env = navi_interaction_environment();

	for (;;) {
		navi_obj expr = call_read(env);
		if (navi_is_eof(expr))
			break;
		navi_write(navi_eval(expr, env), env);
	}
	putchar('\n');
	exit(0);
}

static void program(struct navi_options *options, struct navi_port *p)
{
	navi_env env = navi_empty_environment();
	navi_set_command_line(options->argv, env);
	while (!navi_is_eof(navi_eval(navi_read(p, env), env)))
		/* nothing */;
}

int main(int argc, char *argv[])
{
	struct navi_options options = {
		.filename = NULL,
		.argv = &argv[argc],
	};
	parse_opts(argc, argv, &options);
	navi_init();
	if (options.filename) {
		navi_obj port;
		navi_env env = navi_empty_environment();
		if (!strcmp(options.filename, "-")) {
			port = navi_make_file_input_port(stdin);
		} else {
			navi_obj filename = navi_cstr_to_string(options.filename);
			port = navi_open_input_file(filename, env);
		}
		navi_env_unref(env);
		program(&options, navi_port(port));
	} else {
		repl(&options);
	}
}
