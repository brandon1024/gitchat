#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <unistd.h>

#include "run-command.h"
#include "utils.h"

static int execute(struct child_process_def *cmd, int capture,
		struct strbuf *buffer);

void child_process_def_init(struct child_process_def *cmd)
{
	cmd->git_cmd = 0;
	cmd->discard_out = 0;

	argv_array_init(&cmd->args);
}

void child_process_def_release(struct child_process_def *cmd)
{
	argv_array_release(&cmd->args);
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

static int execute(struct child_process_def *cmd, int capture,
		struct strbuf *buffer)
{
	const size_t BUFF_LEN = 1024;
	char buff[BUFF_LEN];

	char *command;
	if(cmd->discard_out)
		argv_array_push(&cmd->args, "&> /dev/null", NULL);

	if(cmd->git_cmd)
		argv_array_prepend(&cmd->args, "git", NULL);

	command = argv_array_collapse(&cmd->args);
	if(command == NULL)
		BUG("Unexpected child_process_def with no arguments.");

	LOG_TRACE("executing shell process '%s'", command);

	FILE *fp = popen(command, "r");
	free(command);

	if(fp == NULL)
		FATAL("Unable to create pipe to shell process.");

	while(fgets(buff, BUFF_LEN, fp) != NULL) {
		if(capture)
			strbuf_attach(buffer, buff, BUFF_LEN);
		else
			fprintf(stdout, "%s", buff);
	}

	int status = pclose(fp);
	if(status == -1)
		FATAL("Unable to close pipe to shell process.");

	return status;
}
