#include <string.h>
#include <unistd.h>
#include <signal.h>
#include <sys/wait.h>
#include <fcntl.h>

#include "paging.h"
#include "run-command.h"
#include "fs-utils.h"
#include "utils.h"

static struct child_process_def cmd;

static int get_pager(struct strbuf *);
static void register_sigchld_handler(void (*handler)(int));
static void sigchld_handler(int);

void pager_start()
{
	if (!isatty(STDOUT_FILENO))
		DIE("output stream is not a TTY; cannot display paged content");
	if (!isatty(STDERR_FILENO))
		WARN("error stream is not a TTY; content might not be displayed correctly");

	struct strbuf pager_executable;
	strbuf_init(&pager_executable);

	if (get_pager(&pager_executable))
		FATAL("unable to display paged output; no pager executable could be found.");

	child_process_def_init(&cmd);
	str_array_push(&cmd.env, "LESS=FRX", "LV=-c", "MORE=FRX", NULL);
	cmd.executable = pager_executable.buff;

	child_process_def_stdin(&cmd, STDIN_PROVISIONED);
	if (pipe(cmd.in_fd) < 0)
		FATAL("pipe() failed unexpectedly.");

	register_sigchld_handler(sigchld_handler);

	start_command(&cmd);
	cmd.executable = NULL;

	if (dup2(cmd.in_fd[1], STDOUT_FILENO) < 0)
		FATAL("dup2() failed unexpectedly.");
	if (dup2(cmd.in_fd[1], STDERR_FILENO) < 0)
		FATAL("dup2() failed unexpectedly.");

	close(cmd.in_fd[0]);

	strbuf_release(&pager_executable);
}

void pager_stop()
{
	register_sigchld_handler(SIG_DFL);

	close(STDOUT_FILENO);
	close(STDERR_FILENO);
	close(cmd.in_fd[1]);

	finish_command(&cmd);
	child_process_def_release(&cmd);

	int stdin_fd = open("/proc/self/fd/1", O_WRONLY);
	if (stdin_fd < 0)
		FATAL("unable to open /proc/self/fd/1");
	if (dup2(stdin_fd, STDOUT_FILENO) < 0)
		FATAL("dup2() failed unexpectedly.");

	int stdout_fd = open("/proc/self/fd/2", O_WRONLY);
	if (stdout_fd < 0)
		FATAL("unable to open /proc/self/fd/2");
	if (dup2(stdout_fd, STDERR_FILENO) < 0)
		FATAL("dup2() failed unexpectedly.");
}

/**
 * Try to locate an executable pager on the system, and place the name of the
 * executable into the pager strbuf if found.
 *
 * The pager that will be used is chosen in the following order:
 * - GIT_CHAT_PAGER environment variable
 * - GIT_PAGER environment variable
 * - PAGER environment variable
 * - less
 * - more
 * - cat
 *
 * If a pager cannot be found or is not executable, falls back to the next one
 * in the list. Returns zero if a suitable pager was found, otherwise returns 1.
 * */
static int get_pager(struct strbuf *pager)
{
	const char *env = NULL;
	const char * const recognized_vars[] = {"GIT_CHAT_PAGER", "GIT_PAGER", "PAGER", NULL};
	for (size_t i = 0; recognized_vars[i]; i++) {
		env = getenv(*recognized_vars);
		if (env && *env) {
			strbuf_attach_str(pager, env);
			env = recognized_vars[i];
			break;
		}
	}

	if (pager->len && !is_executable(pager->buff)) {
		WARN("%s defined through %s environment variable is not executable;"
			 "falling back to another pager", pager->buff, env);
		strbuf_clear(pager);
	} else if (pager->len) {
		return 0;
	}

	char *pager_path = find_in_path("less");
	if (pager_path) {
		strbuf_attach_str(pager, pager_path);
		free(pager_path);
		return 0;
	}

	pager_path = find_in_path("more");
	if (pager_path) {
		strbuf_attach_str(pager, pager_path);
		free(pager_path);
		return 0;
	}

	pager_path = find_in_path("cat");
	if (pager_path) {
		strbuf_attach_str(pager, pager_path);
		free(pager_path);
		return 0;
	}

	return 1;
}

/**
 * SIGCHLD handler used to exit git-chat if the pager child process terminates.
 * */
static void sigchld_handler(int sig)
{
	(void)sig;
	int status;

	pid_t ret = waitpid(cmd.pid, &status, WNOHANG);
	if (ret < 0)
		FATAL("waitpid() failed unexpectedly.");
	if (ret != cmd.pid)
		return;

	close(cmd.in_fd[1]);
	exit(status);
}

/**
 * Register a handler for the SIGCHLD signal.
 * */
static void register_sigchld_handler(void (*handler)(int))
{
	struct sigaction sa;

	memset(&sa, 0, sizeof(sa));
	sa.sa_handler = handler;

	sigaction(SIGCHLD, &sa, NULL);
}
