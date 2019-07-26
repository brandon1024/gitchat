#include <stdlib.h>
#include <stdio.h>
#include <stdarg.h>
#include <string.h>
#include <errno.h>
#include <sys/stat.h>

#include "utils.h"
#include "run-command.h"
#include "argv-array.h"

static void print_message(FILE *output_stream, const char *prefix,
		const char *fmt, va_list varargs);
static NORETURN void default_exit_routine(int status);
static NORETURN void (*exit_routine)(int) = default_exit_routine;

NORETURN void BUG(const char *fmt, ...)
{
	va_list varargs;

	va_start(varargs, fmt);
	print_message(stderr, "BUG: ", fmt, varargs);
	va_end(varargs);

	exit_routine(EXIT_FAILURE);
}

NORETURN void FATAL(const char *fmt, ...)
{
	va_list varargs;

	va_start(varargs, fmt);
	print_message(stderr, "Fatal Error: ", fmt, varargs);
	va_end(varargs);

	exit_routine(EXIT_FAILURE);
}

NORETURN void DIE(const char *fmt, ...)
{
	va_list varargs;

	va_start(varargs, fmt);
	vfprintf(stderr, fmt, varargs);
	fprintf(stderr, "\n");
	va_end(varargs);

	if (errno > 0)
		fprintf(stderr, "%s\n", strerror(errno));

	exit_routine(EXIT_FAILURE);
}

void WARN(const char *fmt, ...)
{
	va_list varargs;

	va_start(varargs, fmt);
	print_message(stderr, "Warn: ", fmt, varargs);
	va_end(varargs);
}

static void print_message(FILE *output_stream, const char *prefix,
		const char *fmt, va_list varargs)
{
	fprintf(output_stream, "%s", prefix);
	vfprintf(output_stream, fmt, varargs);
	fprintf(output_stream, "\n");

	if (errno > 0)
		fprintf(stderr, "%s\n", strerror(errno));
}

void set_exit_routine(NORETURN void (*new_exit_routine)(int))
{
	exit_routine = new_exit_routine;
}

static NORETURN void default_exit_routine(int status)
{
	exit(status);
}

int is_inside_git_chat_space()
{
	int errsv = errno;

	struct stat sb;
	if (stat(".git-chat", &sb) == -1 || !S_ISDIR(sb.st_mode)) {
		LOG_DEBUG("Cannot stat .git-chat directory; %s", strerror(errno));
		errno = errsv;
		return 0;
	}

	if (stat(".git", &sb) == -1 || !S_ISDIR(sb.st_mode)) {
		LOG_DEBUG("Cannot stat .git directory; %s", strerror(errno));
		errno = errsv;
		return 0;
	}

	struct child_process_def cmd;
	child_process_def_init(&cmd);
	cmd.git_cmd = 1;
	cmd.no_in = 1;
	cmd.no_out = 1;
	cmd.no_err = 1;
	argv_array_push(&cmd.args, "rev-parse", "--is-inside-work-tree", NULL);

	//if 'git rev-parse --is-inside-work-tree' return with a zero exit status,
	//its safe enough to assume we are in a git-chat space.
	int status = run_command(&cmd);
	child_process_def_release(&cmd);

	errno = errsv;
	return status == 0;
}
