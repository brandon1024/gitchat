#include <stdlib.h>
#include <stdio.h>
#include <errno.h>
#include <string.h>

#include "run-command.h"

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
	if(cmd->git_cmd)
		argv_array_prepend(&cmd->args, "git", NULL);

	command = argv_array_collapse(&cmd->args);

	if(cmd->discard_out) {
		int ret = system(command);
		free(command);

		if(ret == -1) {
			fprintf(stderr, "fatal error: unable to run command.\n");
			return 1;
		}

		return 0;
	}

	FILE *fp = popen(command, "r");
	free(command);

	if(fp == NULL) {
		fprintf(stderr, "fatal error: unable to create pipe to shell process. "
				"%x: %s\n", errno, strerror(errno));
		return 1;
	}

	while(fgets(buff, BUFF_LEN, fp) != NULL) {
		if(capture)
			strbuf_attach(buffer, buff, BUFF_LEN);
		else
			fprintf(stdout, "%s", buff);
	}

	int status = pclose(fp);
	if(status == -1) {
		fprintf(stderr, "fatal error: unable to close pipe to shell process. "
				"%x: %s\n", errno, strerror(errno));
		return 1;
	}

	return 0;
}