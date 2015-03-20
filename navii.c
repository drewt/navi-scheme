/* Copyright (c) 2014-2015, Drew Thoreson
 * All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions are met:
 *
 * 1. Redistributions of source code must retain the above copyright notice,
 *    this list of conditions and the following disclaimer.
 *
 * 2. Redistributions in binary form must reproduce the above copyright
 *    notice, this list of conditions and the following disclaimer in the
 *    documentation and/or other materials provided with the distribution.
 *
 * THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS"
 * AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
 * IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
 * ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT HOLDER OR CONTRIBUTORS BE
 * LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR
 * CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF
 * SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS
 * INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN
 * CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE)
 * ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE
 * POSSIBILITY OF SUCH DAMAGE.
*/

#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <setjmp.h>
#include <getopt.h>

#include "navi.h"

static _Noreturn void usage(const char *name, int status)
{
	printf("\
usage: %s [OPTION ...] [FILENAME [ARGUMENT ...]]\n\
\n\
  FILENAME is a Scheme source file, or '-' to read from standard input.\n\
  OPTION may be one of the following:\n\
\n\
    -L, --lib-path PATHNAME  add PATHNAME to the library search paths\n\
    -h, --help               display this text and exit\n\
        --version            display version and exit\n", name);
	exit(status);
}

static void version(void)
{
	puts(PACKAGE_NAME " " NAVI_VERSION);
	puts("(c) 2014-2015 Drew Thoreson");
}

static struct option long_options[] = {
	{ "lib-path", required_argument, 0, 'L' },
	{ "help",     no_argument,       0, 'h' },
	{ "version",  no_argument,       0, 'V' },
};

struct navi_options {
	char **argv;
	char *filename;
	navi_env env;
};

void parse_opts(int argc, char *argv[], struct navi_options *options)
{
	int options_index = 0;
	navi_obj cons, lib_paths = navi_make_nil();
	for (;;) {
		int c = getopt_long(argc, argv, "L:h", long_options, &options_index);
		if (c < 0)
			break;
		switch (c) {
		case 'L':
			lib_paths = navi_make_pair((navi_obj) { .v = optarg },
						lib_paths);
			break;
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

	options->argv = &argv[optind];
	options->filename = argv[optind];

	navi_list_for_each(cons, lib_paths) {
		navi_add_lib_search_path(navi_car(cons).v, options->env);
	}
}

static navi_obj call_read(navi_env env)
{
	return navi_read(navi_port(navi_current_input_port(env)), env);
}

static _Noreturn void repl(struct navi_options *options)
{
	navi_env env = _navi_interaction_environment(options->env);
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

static void program(struct navi_options *options, navi_obj port)
{
	navi_env env = options->env;
	struct navi_guard *guard = navi_gc_guard(port, env);
	navi_set_command_line(options->argv, env);
	while (!navi_is_eof(navi_eval(navi_read(navi_port(port), env), env)))
		/* nothing */;
	navi_gc_unguard(guard);
}

int main(int argc, char *argv[])
{
	navi_init();
	struct navi_options options = {
		.argv      = &argv[argc],
		.filename  = NULL,
		.env       = navi_empty_environment(),
	};
	parse_opts(argc, argv, &options);
	if (options.filename) {
		navi_obj port;
		if (!strcmp(options.filename, "-")) {
			port = navi_make_file_input_port(stdin);
		} else {
			navi_obj filename = navi_cstr_to_string(options.filename);
			port = navi_open_input_file(filename, options.env);
		}
		program(&options, port);
	} else {
		repl(&options);
	}
}
