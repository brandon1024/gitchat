#ifndef GIT_CHAT_RUN_COMMAND_H
#define GIT_CHAT_RUN_COMMAND_H

#include "argv-array.h"
#include "strbuf.h"

#define STDIN_INHERITED		(1 << 0)
#define STDIN_PROVISIONED	(1 << 1)
#define STDIN_NULL			(1 << 2)

#define STDOUT_INHERITED	(1 << 4)
#define STDOUT_PROVISIONED	(1 << 5)
#define STDOUT_NULL			(1 << 6)

#define STDERR_INHERITED	(1 << 8)
#define STDERR_PROVISIONED	(1 << 9)
#define STDERR_NULL			(1 << 10)

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
 * . std_fd_info
 * 		Set of flags that indicate how the child process std streams should behave.
 * 		Flags are bitwise OR'ed.
 * . in_fd
 * 		If STDIN_PROVISIONED, should be the fd pair for a pipe
 * . out_fd
 * 		If STDOUT_PROVISIONED, should be the fd pair for a pipe
 * . err_fd
 * 		If STDERR_PROVISIONED, should be the fd pair for a pipe
 *
 *
 * Managing Standard Streams:
 * Each standard stream has three modes: INHERITED, PROVISIONED, and NULL.
 *
 * - If INHERITED, the child process will inherit the standard streams from the parent
 * process.
 * - If PROVISIONED, the standard streams are redirected to the file
 * descriptors in the child_process_def structure (in_fd, out_fd, err_fd).
 * - If NULL, the standard streams for the child process are redirected to/from /dev/null.
 * */

struct child_process_def_internal {
	int notify_pipe[2];
};

struct child_process_def {
	pid_t pid;
	const char *dir;
	const char *executable;
	struct argv_array args;
	struct str_array env;
	unsigned int git_cmd: 1;
	unsigned int use_shell: 1;
	unsigned int std_fd_info;
	int in_fd[2];
	int out_fd[2];
	int err_fd[2];
	struct child_process_def_internal internals;
};

/**
 * Initialize a child_process_def to default values. After use, the child_process_def
 * should be child_process_def_release()d.
 * */
void child_process_def_init(struct child_process_def *cmd);

/**
 * Set the stdin flag without changing the stdout or stderr flags.
 * */
void child_process_def_stdin(struct child_process_def *cmd, unsigned int flag);

/**
 * Set the stdout flag without changing the stdin or stderr flags.
 * */
void child_process_def_stdout(struct child_process_def *cmd, unsigned int flag);

/**
 * Set the stderr flag without changing the stdin or stdout flags.
 * */
void child_process_def_stderr(struct child_process_def *cmd, unsigned int flag);

/**
 * Release any resources tracked under a child_process_def, and re-initialize
 * the child_process_def.
 *
 * If any pipes were provisioned, they must be closed manually by the caller since
 * this child_process_def_release() will not close them.
 * */
void child_process_def_release(struct child_process_def *cmd);

/**
 * Run an executable, as described by the child_process_def, but wait for the
 * child process to terminate.
 *
 * The child process will inherit all standard streams from the parent process,
 * however this can be manipulated in the child_process_def.
 *
 * This function must only be used when standard streams are inherited or null;
 * using provisioned streams can cause this function to block indefinitely if the
 * pipe to/from the child process has been filled.
 *
 * Returns the exit status of the command.
 * */
int run_command(struct child_process_def *cmd);

/**
 * Run an executable, as described by the child_process_def, and return the pid
 * of the child process. This is primarily used to run executables with provisioned
 * standard streams.
 *
 * After start_command() is invoked, the finish_command() function should be
 * called after the standard streams are written to or read from to close any
 * open pipes.
 * */
int start_command(struct child_process_def *cmd);

/**
 * Finish running a command by waiting for the child process to exit. Once the
 * child process has terminated, closes any open pipes, and returns the exit
 * status of the child process.
 *
 * Note that the caller must manually close any provisioned pipes.
 * */
int finish_command(struct child_process_def *cmd);

/**
 * Run a command, as described by the child_process_def, but capture the command
 * stdout to the given strbuf.
 *
 * All other standard streams will be inherited from the parent process.
 *
 * Returns the exit status of the command.
 * */
int capture_command(struct child_process_def *cmd, struct strbuf *buffer);

#endif //GIT_CHAT_RUN_COMMAND_H
