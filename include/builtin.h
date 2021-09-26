#ifndef GIT_CHAT_BUILTIN_H
#define GIT_CHAT_BUILTIN_H

/**
 * Builtin API
 *
 * To create a new builtin:
 * - Create <builtin>.c in builtin/
 * - In <builtin>.c, define a function with the following signature:
 *
 *     int cmd_<builtin>(int argc, char *argv[])
 *
 * - Add extern function prototype to builtin.h
 * - Register builtin by adding new member of registered_builtins struct:
 *     { "<builtin>", cmd_<builtin> },
 * */

struct cmd_builtin {
	const char *cmd;
	int (*fn)(int, char **);
};

extern int cmd_channel(int argc, char *argv[]);
extern int cmd_config(int argc, char *argv[]);
extern int cmd_get(int argc, char *argv[]);
extern int cmd_init(int argc, char *argv[]);
extern int cmd_message(int argc, char *argv[]);
extern int cmd_publish(int argc, char *argv[]);
extern int cmd_read(int argc, char *argv[]);
extern int cmd_import_key(int argc, char *argv[]);

struct cmd_builtin registered_builtins[] = {
		{ "channel", cmd_channel },
		{ "config", cmd_config },
		{ "init", cmd_init },
		{ "message", cmd_message },
		{ "publish", cmd_publish },
		{ "get", cmd_get },
		{ "read", cmd_read },
		{ "import-key", cmd_import_key },
		{ NULL, NULL }
};

#endif //GIT_CHAT_BUILTIN_H
