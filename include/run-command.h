#ifndef GIT_CHAT_RUN_COMMAND_H
#define GIT_CHAT_RUN_COMMAND_H

#include "argv-array.h"
#include "strbuf.h"

struct child_process_def {
	struct argv_array args;
	unsigned int discard_out: 1;
	unsigned int git_cmd: 1;
};

/**
 * Initialize a child_process_def to default values.
 * */
void child_process_def_init(struct child_process_def *cmd);

/**
 * Release any resources tracked under a child_process_def, and re-initialize
 * the child_process_def.
 * */
void child_process_def_release(struct child_process_def *cmd);

/**
 * Run a command, as described by the child_process_def.
 *
 * If git_cmd is 1, cmd->args are treated as arguments to the git command.
 * Otherwise, the args is collapsed into a string and executed as-is.
 *
 * If discard_out is 1, the command is executed but the output from the command
 * is not printed to stdout.
 * */
int run_command(struct child_process_def *cmd);

/**
 * Run a command, as described by the child_process_def, but capture the command
 * output to the given strbuf.
 *
 * See run_command() for child_process_def usage details.
 * */
int capture_command(struct child_process_def *cmd, struct strbuf *buffer);

#endif //GIT_CHAT_RUN_COMMAND_H
