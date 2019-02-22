#ifndef GIT_CHAT_BUILTIN_H
#define GIT_CHAT_BUILTIN_H

/**
 * Builtin API
 *
 * To create a new builtin:
 * - Create <builtin>.c in builtin/
 * - In <builtin>.c, define a function with the following signature:
 *
 * int cmd_<builtin>(int argc, char *argv[])
 *
 * - Add extern function prototype to builtin.h
 * - In main.c, add builtin to builtins[]:
 *
 * { "<builtin>", cmd_<builtin> },
 * */

struct cmd_builtin {
	const char *cmd;
	int (*fn)(int, char **);
};

extern int cmd_channel(int argc, char *argv[]);
extern int cmd_message(int argc, char *argv[]);
extern int cmd_publish(int argc, char *argv[]);
extern int cmd_get(int argc, char *argv[]);
extern int cmd_read(int argc, char *argv[]);

#endif //GIT_CHAT_BUILTIN_H
