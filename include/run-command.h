#ifndef GIT_CHAT_RUN_COMMAND_H
#define GIT_CHAT_RUN_COMMAND_H

#include "argv-array.h"
#include "strbuf.h"

/**
 * run-command api
 *
 * The run-command api provides an interface for executing programs and shell
 * commands as child processes, manipulating their standard streams and capturing
 * their output for external processing.
 *
 * Internally, run-command fork()s the current process and execve()s the executable
 * as a child process.
 *
 * `child_process_def` Data Structure:
 * . pid_t pid
 * 		The process id for the child process that was forked from the current process.
 * 		This property has no real external usefulness, and is only used by run_command()
 * 		and capture_command().
 * . dir
 * 		The directory from which the executable should be run. This buffer should be
 * 		statically allocated, since it will not be free()d by child_process_def_release.
 * . executable
 * 		Name or full path to the executable that is to be run.
 * . args
 * 		The arguments passed to the executable.
 * . env
 * 		A list of environment variables that will be passed to the child process.
 * 		Variables must have the form "key=value".
 * . git_cmd
 * 		If true, `git` is used as the executable.
 * . use_shell
 * 		Run the executable through '/bin/sh -c'.
 * . no_in
 * 		Use /dev/null as stdin.
 * . no_out
 * 		Use /dev/null as stdout.
 * . no_err
 * 		Use /dev/null as stderr.
 * . stderr_to_stdout
 * 		Redirect all stderr to the stdout stream.
 * */

struct child_process_def {
	pid_t pid;
	const char *dir;
	const char *executable;
	struct argv_array args;
	struct str_array env;
	unsigned int git_cmd: 1;
	unsigned int use_shell: 1;
	unsigned int no_in: 1;
	unsigned int no_out: 1;
	unsigned int no_err: 1;
	unsigned int stderr_to_stdout: 1;
};

/**
 * Initialize a child_process_def to default values. After use, the child_process_def
 * should be child_process_def_release()d.
 * */
void child_process_def_init(struct child_process_def *cmd);

/**
 * Release any resources tracked under a child_process_def, and re-initialize
 * the child_process_def.
 * */
void child_process_def_release(struct child_process_def *cmd);

/**
 * Run an executable, as described by the child_process_def.
 *
 * Returns the exit status of the command.
 * */
int run_command(struct child_process_def *cmd);

/**
 * Run a command, as described by the child_process_def, but capture the command
 * stdout to the given strbuf.
 *
 * Returns the exit status of the command.
 * */
int capture_command(struct child_process_def *cmd, struct strbuf *buffer);

#endif //GIT_CHAT_RUN_COMMAND_H
