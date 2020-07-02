#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <signal.h>

#include "paging.h"
#include "run-command.h"
#include "fs-utils.h"
#include "utils.h"

static struct child_process_def cmd;

static int get_pager(struct strbuf *);

static void pager_stop(int in_sig)
{
	if (!in_sig) {
		fflush(stdout);
		fflush(stderr);
	}

	close(STDOUT_FILENO);
	close(STDERR_FILENO);
	close(cmd.in_fd[1]);

	int status = finish_command(&cmd);
	(void) status;

	child_process_def_release(&cmd);
}

static void pager_stop_exit(void)
{
	pager_stop(0);
}

static void sigaction_register(int signal, void (*handler)(int))
{
	struct sigaction sa;

	memset(&sa, 0, sizeof(sa));
	sa.sa_handler = handler;

	sigaction(signal, &sa, NULL);
}

static void pager_stop_signal(int sig)
{
	(void)sig;
	pager_stop(1);
	sigaction_register(sig, SIG_DFL);
	raise(sig);
}

void pager_start(void)
{
	if (!isatty(STDOUT_FILENO))
		return;

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

	start_command(&cmd);
	close(cmd.in_fd[0]);
	cmd.executable = NULL;

	if (dup2(cmd.in_fd[1], STDOUT_FILENO) < 0)
		FATAL("dup2() failed unexpectedly.");
	if (isatty(STDERR_FILENO)) {
		if (dup2(cmd.in_fd[1], STDERR_FILENO) < 0)
			FATAL("dup2() failed unexpectedly.");
	}

	strbuf_release(&pager_executable);

	sigaction_register(SIGINT, pager_stop_signal);
	sigaction_register(SIGHUP, pager_stop_signal);
	sigaction_register(SIGTERM, pager_stop_signal);
	sigaction_register(SIGQUIT, pager_stop_signal);
	sigaction_register(SIGPIPE, pager_stop_signal);
	atexit(pager_stop_exit);
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
	const char * const recognized_vars[] = {
			"GIT_CHAT_PAGER",
			"GIT_PAGER",
			"PAGER",
			NULL
	};
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
