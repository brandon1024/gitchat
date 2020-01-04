#include <stdlib.h>
#include <stdio.h>
#include <stdarg.h>
#include <string.h>
#include <unistd.h>
#include <errno.h>
#include <sys/stat.h>

#include "utils.h"
#include "run-command.h"
#include "argv-array.h"

static void print_message(FILE *, const char *, const char *, va_list);
static NORETURN void default_exit_routine(int);
static NORETURN void (*exit_routine)(int) = default_exit_routine;

NORETURN void BUG(const char *fmt, ...)
{
	va_list varargs;

	va_start(varargs, fmt);
	print_message(stderr, "BUG", fmt, varargs);
	va_end(varargs);

	exit_routine(EXIT_FAILURE);
}

NORETURN void FATAL(const char *fmt, ...)
{
	va_list varargs;

	va_start(varargs, fmt);
	print_message(stderr, "fatal", fmt, varargs);
	va_end(varargs);

	exit_routine(EXIT_FAILURE);
}

NORETURN void DIE(const char *fmt, ...)
{
	va_list varargs;

	va_start(varargs, fmt);
	print_message(stderr, NULL, fmt, varargs);
	va_end(varargs);

	exit_routine(EXIT_FAILURE);
}

void WARN(const char *fmt, ...)
{
	va_list varargs;

	va_start(varargs, fmt);
	print_message(stderr, "warn", fmt, varargs);
	va_end(varargs);
}

static void print_message(FILE *output_stream, const char *prefix,
		const char *fmt, va_list varargs)
{
	struct strbuf message_buffer;
	strbuf_init(&message_buffer);

	strbuf_attach_vfmt(&message_buffer, fmt, varargs);

	if (prefix) {
		struct str_array lines;
		str_array_init(&lines);
		strbuf_split(&message_buffer, "\n", &lines);
		strbuf_clear(&message_buffer);

		for (size_t i = 0; i < lines.len; i++) {
			const char *line = str_array_get(&lines, i);
			strbuf_attach_fmt(&message_buffer, "%s: %s\n", prefix, line);
		}

		str_array_release(&lines);
	}

	fputs(message_buffer.buff, output_stream);

	if (errno > 0)
		fprintf(stderr, "%s\n", strerror(errno));

	strbuf_release(&message_buffer);
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
	cmd.std_fd_info = STDIN_NULL | STDOUT_NULL | STDERR_NULL;
	argv_array_push(&cmd.args, "rev-parse", "--is-inside-work-tree", NULL);

	//if 'git rev-parse --is-inside-work-tree' return with a zero exit status,
	//its safe enough to assume we are in a git-chat space.
	int status = run_command(&cmd);
	child_process_def_release(&cmd);

	errno = errsv;
	return status == 0;
}

ssize_t recoverable_read(int fd, void *buf, size_t len)
{
	int errsv = errno;

	ssize_t bytes_read = 0;
	while(1) {
		bytes_read = read(fd, buf, len);
		if ((bytes_read < 0) && (errno == EAGAIN || errno == EINTR)) {
			errno = errsv;
			continue;
		}

		break;
	}

	return bytes_read;
}

ssize_t recoverable_write(int fd, const void *buf, size_t len)
{
	int errsv = errno;

	ssize_t bytes_written = 0;
	while(1) {
		bytes_written = write(fd, buf, len);
		if ((bytes_written < 0) && (errno == EAGAIN || errno == EINTR)) {
			errno = errsv;
			continue;
		}

		break;
	}

	return bytes_written;
}
