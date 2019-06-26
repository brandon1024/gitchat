#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <unistd.h>
#include <errno.h>
#include <sys/stat.h>
#include <fcntl.h>

#include "run-command.h"
#include "utils.h"

#define READ 0
#define WRITE 1

extern char **environ;

static int exec_as_child_process(struct child_process_def *cmd, int capture,
		struct strbuf *buffer);
static inline void set_cloexec(int fd);
static void merge_env(struct str_array *deltaenv, struct str_array *result);
static char *find_in_path(const char *file);
static int is_executable(const char *name);

static NORETURN void child_exit_routine(int status);

void child_process_def_init(struct child_process_def *cmd)
{
	cmd->pid = -1;
	cmd->dir = NULL;
	cmd->executable = NULL;
	cmd->no_in = 0;
	cmd->no_out = 0;
	cmd->no_err = 0;
	cmd->stderr_to_stdout = 0;
	cmd->use_shell = 0;
	cmd->git_cmd = 0;

	argv_array_init(&cmd->args);
	str_array_init(&cmd->env);
}

void child_process_def_release(struct child_process_def *cmd)
{
	argv_array_release(&cmd->args);
	str_array_release(&cmd->env);
	child_process_def_init(cmd);
}

int run_command(struct child_process_def *cmd)
{
	return exec_as_child_process(cmd, 0, NULL);
}

int capture_command(struct child_process_def *cmd, struct strbuf *buffer)
{
	return exec_as_child_process(cmd, 1, buffer);
}

/**
 * Execute a command or program in a child process.
 *
 * If capture is 1, the stdout of the child process is captured and placed into
 * the string buffer `buffer`. Otherwise, the child stdout is written to the
 * console.
 * */
static int exec_as_child_process(struct child_process_def *cmd, int capture,
		struct strbuf *buffer)
{
	if (capture && !buffer)
		BUG("Command output capture enabled but buffer is NULL.");
	if (cmd->git_cmd && cmd->executable)
		BUG("Ambiguous child_process_def; git_cmd is true but executable is not NULL");
	if (!cmd->git_cmd && !cmd->executable)
		BUG("Unexpected child_process_def without executable specified.");

	char *executable_path = NULL;
	if (cmd->git_cmd || !strchr(cmd->executable, '/')) {
		executable_path = find_in_path(cmd->git_cmd ? "git" : cmd->executable);
		if (!executable_path)
			FATAL("Executable '%s' could not be found in PATH, or is not executable.", cmd->executable);
	} else {
		executable_path = strdup(cmd->executable);
		if(!executable_path)
			FATAL("Unable to allocate memory.");
	}

	if (cmd->args.arr.len) {
		char *args_literal = argv_array_collapse(&cmd->args);
		LOG_TRACE("executing shell process '%s %s'", executable_path, args_literal);
		free(args_literal);
	} else {
		LOG_TRACE("executing shell process '%s'", executable_path);
	}

	//args and env are duplicated so child_process_def is not modified.
	struct argv_array args;
	argv_array_init(&args);
	for (size_t i = 0; i < cmd->args.arr.len; i++)
		argv_array_push(&args, cmd->args.arr.strings[i], NULL);

	struct str_array env;
	str_array_init(&env);
	for (size_t i = 0; i < cmd->env.len; i++)
		str_array_push(&env, cmd->env.strings[i], NULL);

	merge_env(&cmd->env, &env);

	int in_fd[2];
	int out_fd[2];
	int err_fd[2];
	if(pipe(in_fd) < 0 || pipe(out_fd) < 0 || pipe(err_fd) < 0)
		FATAL("Invocation of pipe() system call failed.");

	int child_ret_status = -1;
	cmd->pid = fork();
	if (cmd->pid == 0) {
		set_exit_routine(&child_exit_routine);

		if (cmd->dir && chdir(cmd->dir))
			FATAL("Unable to chdir to '%s'", cmd->dir);

		/*
		 * Prepare pipes between current (child) process and parent process.
		 * */
		close(in_fd[WRITE]);
		close(out_fd[READ]);
		close(err_fd[READ]);
		set_cloexec(in_fd[READ]);
		set_cloexec(out_fd[WRITE]);
		set_cloexec(err_fd[WRITE]);

		int null_fd = open("/dev/null", O_RDWR | O_CLOEXEC);
		if (null_fd < 0)
			FATAL("Failed to open() /dev/null.");
		set_cloexec(null_fd);

		if (cmd->no_in) {
			close(in_fd[READ]);
			in_fd[READ] = null_fd;
		}

		if (cmd->no_out) {
			close(out_fd[WRITE]);
			out_fd[WRITE] = null_fd;
		}

		if (cmd->no_err) {
			close(err_fd[WRITE]);
			err_fd[WRITE] = null_fd;
		}

		if (dup2(in_fd[READ], 0) < 0)
			FATAL("dup2() failed unexpectedly.");
		if (dup2(out_fd[WRITE], 1) < 0)
			FATAL("dup2() failed unexpectedly.");
		if (dup2(err_fd[WRITE], 2) < 0)
			FATAL("dup2() failed unexpectedly.");

		if (cmd->stderr_to_stdout) {
			if (dup2(1, 2) < 0)
				FATAL("dup2() failed unexpectedly.");
		}

		/*
		 * Prepare arguments. argv[0] must be the path of the executable,
		 * and argv must be NULL terminated.
		 */
		argv_array_prepend(&args, executable_path, NULL);
		str_array_insert_nodup((struct str_array *)&args, args.arr.len, NULL);
		char **argv = args.arr.strings;

		/*
		 * Prepare execve() environment. argp must be NULL terminated.
		 */
		str_array_insert_nodup(&env, env.len, NULL);
		char **argp = env.strings;

		/*
		 * Attempt to exec using the command and arguments. In the event execve()
		 * failed with ENOEXEC, try to interpret the command using 'sh -c'.
		 */
		if (!cmd->use_shell)
			execve(argv[0], argv, argp);

		if (cmd->use_shell || errno == ENOEXEC) {
			if (errno == ENOEXEC)
				LOG_WARN("execve() failed to execute '%s'; attempting to run through 'sh -c'", cmd->executable);

			//prepare arguments
			char *collapsed_arguments = argv_array_collapse(&args);
			argv_array_release(&args);
			argv_array_init(&args);
			argv_array_push(&args, "/bin/sh", "-c", collapsed_arguments, NULL);
			str_array_insert_nodup(&env, env.len, NULL);
			argv = args.arr.strings;

			execve(argv[0], argv, argp);
		}

		FATAL("execve() returned unexpectedly.");
	} else if (cmd->pid > 0) {
		size_t BUFF_LEN = 1024;
		char stdout_buffer[BUFF_LEN];

		close(in_fd[READ]);
		close(out_fd[WRITE]);
		close(err_fd[WRITE]);

		size_t bytes_read = 0;
		while ((bytes_read = read(out_fd[READ], stdout_buffer, BUFF_LEN)) > 0) {
			if (capture)
				strbuf_attach(buffer, stdout_buffer, bytes_read);
			else
				fwrite(stdout_buffer, bytes_read, 1, stdout);
		}

		close(in_fd[WRITE]);
		close(out_fd[READ]);
		close(err_fd[READ]);

		/* spin waiting for process exit or error */
		while (waitpid(cmd->pid, &child_ret_status, 0) < 0 && errno == EINTR);
		if (WIFEXITED(child_ret_status))
			child_ret_status = WEXITSTATUS(child_ret_status);
	} else {
		FATAL("Failed to fork process.");
	}

	free(executable_path);
	argv_array_release(&args);
	str_array_release(&env);

	return child_ret_status;
}

/**
 * Configure the given file descriptor with the FD_CLOEXEC, the close-on-exec,
 * flag, which ensures the file descriptor will automatically be closed after
 * a successful execve(2). If the execve(2) fails, the file descriptor is left
 * open. If left unset, the file descriptor will remain open across an execve(2).
 * */
static inline void set_cloexec(int fd)
{
	int flags = fcntl(fd, F_GETFD);
	if (flags >= 0)
		fcntl(fd, F_SETFD, flags | FD_CLOEXEC);
}

/**
 * Extract the environment variable key into the given string buffer.
 *
 * env_var must be NULL terminated.
 * */
static void env_variable_key(char *env_var, struct strbuf *buff)
{
	char *eq = strchr(env_var, '=');
	struct strbuf c_key;
	strbuf_init(&c_key);

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
	char **env_test = environ;
	struct str_array current_env;
	str_array_init(&current_env);

	while (*env_test) {
		str_array_push(&current_env, *(env_test++), NULL);
	}

	for (size_t i = 0; i < deltaenv->len; i++)
		str_array_insert(result, i, deltaenv->strings[i]);

	str_array_sort(&current_env);
	str_array_sort(result);

	size_t p = 0, c = 0;
	while (p < current_env.len && c < result->len) {
		struct strbuf p_key;
		strbuf_init(&p_key);
		env_variable_key(current_env.strings[p], &p_key);

		struct strbuf c_key;
		strbuf_init(&c_key);
		env_variable_key(result->strings[c], &c_key);

		/* If keys are equal, child variable will take precedence */
		int cmp = strcmp(c_key.buff, p_key.buff);
		if (cmp > 0) {
			str_array_insert(result, 0, current_env.strings[p]);
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
		str_array_push(result, current_env.strings[p++], NULL);

	str_array_release(&current_env);
}

/**
 * Search $PATH for a command.  This emulates the path search that
 * execvp would perform, without actually executing the command so it
 * can be used before fork() to prepare to run a command using
 * execve().
 *
 * The caller should ensure that file contains no directory
 * separators.
 *
 * Returns the path to the command, as found in $PATH or NULL if the
 * command could not be found.  The caller assumes ownership of the memory
 * used to store the resultant path.
 */
static char *find_in_path(const char *file)
{
	const char *p = getenv("PATH");
	struct strbuf buf;

	if (!p || !*p)
		return NULL;

	while (1) {
		const char *end = strchr(p, ':');
		strbuf_init(&buf);

		/* POSIX specifies an empty entry as the current directory. */
		if (end != p) {
			strbuf_attach(&buf, (char *)p, end - p);
			strbuf_attach_chr(&buf, '/');
		}

		strbuf_attach(&buf, (char *)file, strlen(file));

		if (is_executable(buf.buff))
			return strbuf_detach(&buf);

		strbuf_release(&buf);
		if (!*end)
			break;

		p = end + 1;
	}

	return NULL;
}

/**
 * Determine whether a file with the given name and path is executable.
 *
 * Returns 1 if the given file is executable, 0 otherwise.
 * */
static int is_executable(const char *name)
{
	struct stat st;

	int not_executable = stat(name, &st) || !S_ISREG(st.st_mode);
	errno = 0;

	return not_executable ? 0 : st.st_mode & S_IXUSR;
}

static NORETURN void child_exit_routine(int status)
{
	_exit(status);
}