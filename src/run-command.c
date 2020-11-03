#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <errno.h>
#include <fcntl.h>
#include <sys/wait.h>
#include <assert.h>

#include "run-command.h"
#include "fs-utils.h"
#include "utils.h"

#define READ 0
#define WRITE 1
#define BUFF_LEN 1024

extern char **environ;

static void merge_env(struct str_array *deltaenv, struct str_array *result);
static NORETURN void child_exit_routine(int status);

static int child_failure_fd = -1;

void child_process_def_init(struct child_process_def *cmd)
{
	cmd->pid = -1;
	cmd->dir = NULL;
	cmd->executable = NULL;
	cmd->std_fd_info = STDIN_INHERITED | STDOUT_INHERITED | STDERR_INHERITED;
	cmd->use_shell = 0;
	cmd->git_cmd = 0;
	cmd->internals = (struct child_process_def_internal) {
		.notify_pipe = {-1,-1}
	};

	argv_array_init(&cmd->args);
	str_array_init(&cmd->env);
}

void child_process_def_stdin(struct child_process_def *cmd, unsigned int flag) {
	cmd->std_fd_info = (cmd->std_fd_info & 0xff0) | flag;
}

void child_process_def_stdout(struct child_process_def *cmd, unsigned int flag) {
	cmd->std_fd_info = (cmd->std_fd_info & 0xf0f) | flag;
}

void child_process_def_stderr(struct child_process_def *cmd, unsigned int flag) {
	cmd->std_fd_info = (cmd->std_fd_info & 0x0ff) | flag;
}

void child_process_def_release(struct child_process_def *cmd)
{
	argv_array_release(&cmd->args);
	str_array_release(&cmd->env);
}

int run_command(struct child_process_def *cmd)
{
	if ((cmd->std_fd_info & 0x00f) == STDIN_PROVISIONED ||
		(cmd->std_fd_info & 0x0f0) == STDOUT_PROVISIONED ||
		(cmd->std_fd_info & 0xf00) == STDERR_PROVISIONED)
		BUG("cannot invoke run_command() on a child_process_def that has provisioned streams");

	start_command(cmd);
	return finish_command(cmd);
}

int capture_command(struct child_process_def *cmd, struct strbuf *buffer)
{
	if (cmd->pid != -1)
		BUG("child_process_def must have a pid of -1; either the pid was modified "
			"or the run-command api was not used correctly");
	if ((cmd->std_fd_info & 0x00f) == STDIN_PROVISIONED ||
		(cmd->std_fd_info & 0xf00) == STDERR_PROVISIONED)
		BUG("cannot invoke capture_command() on a child_process_def that has provisioned streams");

	if ((cmd->std_fd_info & 0x0f0) == STDOUT_NULL)
		BUG("capture_command with STDOUT_NULL definition doesn't make sense");

	if (pipe(cmd->out_fd) < 0)
		FATAL("invocation of pipe() system call failed.");

	child_process_def_stdout(cmd, STDOUT_PROVISIONED);

	start_command(cmd);
	close(cmd->out_fd[WRITE]);

	char out_buffer[BUFF_LEN];
	ssize_t bytes_read = 0;
	while ((bytes_read = xread(cmd->out_fd[READ], out_buffer, BUFF_LEN)) > 0)
		strbuf_attach(buffer, out_buffer, bytes_read);

	if (bytes_read < 0)
		FATAL("failed to read from pipe to child process.");

	close(cmd->out_fd[READ]);

	return finish_command(cmd);
}

int start_command(struct child_process_def *cmd)
{
	if (cmd->pid != -1)
		BUG("child_process_def must have a pid of -1; either the pid was modified "
				"or the run-command api was not used correctly");
	if (cmd->git_cmd && cmd->executable)
		BUG("ambiguous child_process_def; git_cmd is true but executable is not NULL");
	if (!cmd->git_cmd && !cmd->executable)
		BUG("unexpected child_process_def without executable specified.");

	char *executable_path = NULL;
	if (cmd->git_cmd || !strchr(cmd->executable, '/')) {
		executable_path = find_in_path(cmd->git_cmd ? "git" : cmd->executable);
		if (!executable_path)
			FATAL("executable '%s' could not be found in PATH, or is not executable.", cmd->executable);
	} else {
		executable_path = strdup(cmd->executable);
		if (!executable_path)
			FATAL(MEM_ALLOC_FAILED);
	}

	if (cmd->args.arr.len) {
		char *args_literal = argv_array_collapse(&cmd->args);
		LOG_TRACE("executing process '%s %s'", executable_path, args_literal);
		free(args_literal);
	} else {
		LOG_TRACE("executing process '%s'", executable_path);
	}

	//args and env are duplicated so child_process_def is not modified.
	struct argv_array args;
	argv_array_init(&args);
	for (size_t i = 0; i < cmd->args.arr.len; i++) {
		char *string_to_copy = str_array_get((struct str_array *)&cmd->args, i);
		argv_array_push(&args, string_to_copy, NULL);
	}

	struct str_array env;
	str_array_init(&env);

	merge_env(&cmd->env, &env);

	/*
	 * Setup pipe used to notify the parent event if the child process
	 * failed before execve() was called.
	 */
	if (pipe(cmd->internals.notify_pipe) < 0)
		FATAL("invocation of pipe() system call failed.");

	cmd->pid = fork();
	if (cmd->pid == 0) {
		close(cmd->internals.notify_pipe[READ]);
		set_cloexec(cmd->internals.notify_pipe[WRITE]);

		child_failure_fd = cmd->internals.notify_pipe[WRITE];
		set_exit_routine(&child_exit_routine);

		if (cmd->dir && chdir(cmd->dir))
			FATAL("unable to chdir to '%s'", cmd->dir);

		int null_fd = open("/dev/null", O_RDWR | O_CLOEXEC);
		if (null_fd < 0)
			FATAL(FILE_OPEN_FAILED, "/dev/null");
		set_cloexec(null_fd);

		/* Configure stdin */
		switch (cmd->std_fd_info & 0x00f) {
			case STDIN_INHERITED:
				break;
			case STDIN_PROVISIONED:
				close(cmd->in_fd[WRITE]);
				set_cloexec(cmd->in_fd[READ]);
				if ((cmd->in_fd[READ] = dup2(cmd->in_fd[READ], STDIN_FILENO)) < 0)
					FATAL("dup2() failed unexpectedly.");
				break;
			case STDIN_NULL:
				if ((cmd->in_fd[READ] = dup2(null_fd, STDIN_FILENO)) < 0)
					FATAL("dup2() failed unexpectedly.");
				break;
			default:
				BUG("unexpected stream configuration for stdin (%#x).", cmd->std_fd_info & 0x00f);
		}

		/* Configure stdout */
		switch (cmd->std_fd_info & 0x0f0) {
			case STDOUT_INHERITED:
				break;
			case STDOUT_PROVISIONED:
				close(cmd->out_fd[READ]);
				set_cloexec(cmd->out_fd[WRITE]);
				if ((cmd->out_fd[WRITE] = dup2(cmd->out_fd[WRITE], STDOUT_FILENO)) < 0)
					FATAL("dup2() failed unexpectedly.");
				break;
			case STDOUT_NULL:
				if ((cmd->out_fd[WRITE] = dup2(null_fd, STDOUT_FILENO)) < 0)
					FATAL("dup2() failed unexpectedly.");
				break;
			default:
				BUG("unexpected stream configuration for stdout (%#x).", cmd->std_fd_info & 0x0f0);
		}

		/* Configure stderr */
		switch (cmd->std_fd_info & 0xf00) {
			case STDERR_INHERITED:
				break;
			case STDERR_PROVISIONED:
				set_cloexec(cmd->err_fd[WRITE]);
				close(cmd->err_fd[READ]);
				if ((cmd->err_fd[WRITE] = dup2(cmd->err_fd[WRITE], STDERR_FILENO)) < 0)
					FATAL("dup2() failed unexpectedly.");
				break;
			case STDERR_NULL:
				if ((cmd->err_fd[WRITE] = dup2(null_fd, STDERR_FILENO)) < 0)
					FATAL("dup2() failed unexpectedly.");
				break;
			default:
				BUG("unexpected stream configuration for stderr (%#x).", cmd->std_fd_info & 0xf00);
		}

		/*
		 * Prepare arguments. argv[0] must be the path of the executable,
		 * and argv must be NULL terminated.
		 */
		argv_array_prepend(&args, executable_path, NULL);

		/*
		 * Prepare execve() environment. argp must be NULL terminated.
		 */
		char **argp = str_array_detach(&env, NULL);

		/*
		 * Attempt to exec using the command and arguments. In the event execve()
		 * failed with ENOEXEC, try to interpret the command using 'sh -c'.
		 */
		if (!cmd->use_shell) {
			char **argv = argv_array_detach(&args, NULL);
			execve(argv[0], argv, argp);
		}

		if (cmd->use_shell || errno == ENOEXEC) {
			if (errno == ENOEXEC)
				LOG_WARN("execve() failed to execute '%s'; attempting to run through 'sh -c'", cmd->executable);

			char *collapsed_args = argv_array_collapse(&args);
			argv_array_release(&args);

			argv_array_init(&args);
			argv_array_push(&args, "/bin/sh", "-c", collapsed_args, NULL);
			char **argv = argv_array_detach(&args, NULL);

			execve(argv[0], argv, argp);
		}

		FATAL("execve() returned unexpectedly.");
	} else if (cmd->pid < 0) {
		FATAL("failed to fork process.");
	}

	close(cmd->internals.notify_pipe[WRITE]);

	free(executable_path);
	argv_array_release(&args);
	str_array_release(&env);

	LOG_TRACE("child process successfully created with pid %d", cmd->pid);

	return cmd->pid;
}

int finish_command(struct child_process_def *cmd)
{
	if (cmd->pid == -1)
		BUG("child_process_def must not have a pid of -1; either the pid was modified "
			"or the run-command api was not used correctly");

	int child_ret_status = -1;
	int status = 0;

	/* spin waiting for process exit or error */
	while (waitpid(cmd->pid, &child_ret_status, 0) < 0 && errno == EINTR);
	if (WIFEXITED(child_ret_status)) {
		child_ret_status = WEXITSTATUS(child_ret_status);
		LOG_TRACE("child process %d terminated with status %d", cmd->pid, child_ret_status);
	}
	if (WIFSIGNALED(child_ret_status))
		LOG_WARN("child process with pid %d terminated with signal %d", cmd->pid, WTERMSIG(child_ret_status));

	if (xread(cmd->internals.notify_pipe[READ], &status, sizeof(status)) > 0)
		FATAL("child process encountered a fatal error and exited with status %d.", status);

	close(cmd->internals.notify_pipe[READ]);
	child_failure_fd = -1;
	cmd->pid = -1;

	return child_ret_status;
}

/**
 * Extract the environment variable key into the given string buffer.
 *
 * env_var must be NULL terminated.
 * */
static void env_variable_key(char *env_var, struct strbuf *buff)
{
	char *eq = strchr(env_var, '=');

	strbuf_attach(buff, env_var, eq ? eq - env_var : strlen(env_var));
}

/**
 * Merge the current process environment into the array of desired environment
 * variables for the child process.
 *
 * Variables in the desired child process that also exist in the current process
 * will take precedence.
 *
 * 'deltaenv' remains untouched. 'result' must be an empty str_array.
 * */
static void merge_env(struct str_array *deltaenv, struct str_array *result)
{
	char **parent_env = environ;
	struct str_array current_env;
	str_array_init(&current_env);

	if (result->len)
		BUG("merge_env() accepts an empty str_array as an argument, but given str_array was not empty.");

	while (*parent_env)
		str_array_push(&current_env, *(parent_env++), NULL);

	for (size_t i = 0; i < deltaenv->len; i++)
		str_array_insert(result, deltaenv->entries[i].string, i);

	str_array_sort(&current_env);
	str_array_sort(result);

	size_t p = 0, c = 0;
	while (p < current_env.len && c < result->len) {
		struct strbuf p_key;
		strbuf_init(&p_key);
		env_variable_key(current_env.entries[p].string, &p_key);

		struct strbuf c_key;
		strbuf_init(&c_key);
		env_variable_key(result->entries[c].string, &c_key);

		/* If keys are equal, child variable will take precedence */
		int cmp = strcmp(c_key.buff, p_key.buff);
		if (cmp > 0) {
			str_array_insert(result, current_env.entries[p].string, 0);
			c++, p++;
		} else if (!cmp)
			p++, c++;
		else
			c++;

		strbuf_release(&p_key);
		strbuf_release(&c_key);
	}

	/* Add any remaining variables from parent */
	while (p < current_env.len)
		str_array_push(result, current_env.entries[p++].string, NULL);

	str_array_release(&current_env);
}

static NORETURN void child_exit_routine(int status)
{
	ssize_t ret = write(child_failure_fd, &status, sizeof(status));
	assert(ret);
	(void) ret;
	_exit(status);
}
