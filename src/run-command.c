#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <unistd.h>
#include <errno.h>
#include <sys/stat.h>

#include "run-command.h"
#include "utils.h"

#define READ 0
#define WRITE 0

static int execute(struct child_process_def *cmd, int capture,
		struct strbuf *buffer);
static inline void close_pair(int fd[2]);
static void merge_env(struct str_array *deltaenv, struct str_array *result);
static char *find_in_path(const char *file);
static int is_executable(const char *name);
static NORETURN void child_exit_routine(int status);

void child_process_def_init(struct child_process_def *cmd)
{
	cmd->pid = -1;
	cmd->git_cmd = 0;
	cmd->discard_out = 0;
	cmd->executable = NULL;
	cmd->dir = NULL;

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
	return execute(cmd, 0, NULL);
}

int capture_command(struct child_process_def *cmd, struct strbuf *buffer)
{
	return execute(cmd, 1, buffer);
}

/**
 * TODO DOCUMENT ME
 * */
static int execute(struct child_process_def *cmd, int capture,
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

	if(pipe(in_fd) != 0 || pipe(out_fd) != 0 || pipe(err_fd) != 0)
		FATAL("Invocation of pipe() system call failed.");

	cmd->pid = fork();
	if (cmd->pid == 0) {
		set_exit_routine(&child_exit_routine);

		if (cmd->dir && chdir(cmd->dir))
			FATAL("Unable to chdir to '%s'", cmd->dir);

		//todo set up pipes

		/*
		 * Prepare arguments. argv[0] must be the path of the executable,
		 * and argv must be NULL terminated.
		 */
		argv_array_prepend(&args, executable_path, NULL);
		str_array_insert_nodup(&args.arr, args.arr.len, NULL);
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
		execve(argv[0], argv, argp);
		if (errno == ENOEXEC) {
			LOG_TRACE("execve() failed to execute '%s'; attempting to run through 'sh -c'", cmd->executable);

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
		//parent
	} else {
		FATAL("Failed to fork process.");
	}

	free(executable_path);
	argv_array_release(&args);
	str_array_release(&env);

	close_pair(in_fd);
	close_pair(out_fd);
	close_pair(err_fd);

	return 0;
}

static inline void close_pair(int fd[2])
{
	close(fd[0]);
	close(fd[1]);
}

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
	extern char **environ;
	struct str_array current_env;
	str_array_init(&current_env);
	while (*environ)
		str_array_push(&current_env, *(environ++), NULL);

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