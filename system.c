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
#include <time.h>
#include "navi.h"

void navi_set_command_line(char *argv[], navi_env env)
{
	navi_obj head, cons;
	head = cons = navi_make_pair(navi_make_nil(), navi_make_nil());
	for (int i = 0; argv[i]; i++) {
		navi_obj str = navi_cstr_to_string(argv[i]);
		navi_obj next = navi_make_pair(str, navi_make_nil());
		navi_set_cdr(cons, next);
		cons = navi_cdr(cons);
	}
	navi_scope_set(navi_get_global_env(env).lexical,
			navi_sym_command_line, navi_cdr(head));
}

DEFUN(command_line, "command-line", 0, 0)
{
	navi_obj cl = navi_env_lookup(scm_env.lexical, navi_sym_command_line);
	if (!navi_is_pair(cl))
		return navi_make_nil();
	return cl;
}

_Noreturn void navi_exit(navi_obj obj)
{
	int status;
	switch (navi_type(obj)) {
	case NAVI_BOOL:
		status = navi_bool(obj) ? EXIT_SUCCESS : EXIT_FAILURE;
		break;
	case NAVI_NUM:
		status = navi_num(obj);
		break;
	default:
		status = EXIT_FAILURE;
		break;
	}
	_Exit(status);
}

DEFUN(emergency_exit, "emergency-exit", 0, NAVI_PROC_VARIADIC)
{
	navi_exit(scm_nr_args > 0 ? scm_arg1 : navi_make_bool(true));
}

DEFUN(exit, "exit", 0, NAVI_PROC_VARIADIC)
{
	// TODO: run all outstanding dynamic-wind *after* procedures
	navi_exit(scm_nr_args > 0 ? scm_arg1 : navi_make_bool(true));
}

DEFUN(get_environment_variable, "get-environment-variable", 1, 0, NAVI_STRING)
{
	char *var = getenv((char*)navi_string(scm_arg1)->data);
	if (!var)
		return navi_make_bool(false);
	return navi_cstr_to_string(var);
}

extern char **environ;

static navi_obj envvar_to_pair(const char *var)
{
	navi_obj name, value;
	size_t len = strcspn(var, "=");
	char *dup = navi_critical_malloc(len+1);
	memcpy(dup, var, len);
	dup[len] = '\0';
	name = navi_cstr_to_string(dup);
	value = navi_cstr_to_string(var + len + 1);
	free(dup);
	return navi_make_pair(name, value);
}

DEFUN(get_environment_variables, "get-environment-variables", 0, 0)
{
	navi_obj head, cons;
	head = cons = navi_make_pair(navi_make_nil(), navi_make_nil());
	for (int i = 0; environ[i]; i++) {
		navi_obj next = navi_make_pair(envvar_to_pair(environ[i]),
						navi_make_nil());
		navi_set_cdr(cons, next);
		cons = navi_cdr(cons);
	}
	return navi_cdr(head);
}

DEFUN(current_second, "current-second", 0, 0)
{
	// FIXME: supposed to return an inexact number
	return navi_make_num(time(NULL));
}

DEFUN(current_jiffy, "current-jiffy", 0, 0)
{
	struct timespec t;
	static time_t first_second = 0;

	if (clock_gettime(CLOCK_REALTIME, &t) < 0)
		navi_error(scm_env, "unable to read system clock");

	// start at (approximately) 0 to increase the range before overflow
	if (!first_second)
		first_second = t.tv_sec;
	t.tv_sec -= first_second;

	return navi_make_num(t.tv_sec*1000 + t.tv_nsec/1000000);
}

DEFUN(jiffies_per_second, "jiffies-per-second", 0, 0)
{
	return navi_make_num(100);
}
