#include <stdio.h>
#include <stdarg.h>

#include "usage.h"

static const struct usage_description message_cmd_usage[] = {
		USAGE("git chat message [-p | --public] [--recipient <email>...] [(-c | --comment) <comment>] [-m | --message] <message>"),
		USAGE("git chat message (-s | --symmetric) [--password <password>] <message>"),
		USAGE("git chat message (-f | --file) <filename>"),
		USAGE_END()
};

static const struct option_description message_cmd_options[] = {
		OPT_BOOL('p', "public", "encrypt message using public-key (asymmetric) cryptography"),
		OPT_LONG_STRING("recipient", "email", "specify one or more recipients that may decrypt the message"),
		OPT_STRING('c', "comment", "comment", "attach a comment to the message"),
		OPT_STRING('m', "message", "message", "provide the message contents directly through the command line"),
		OPT_BOOL('s', "symmetric", "encrypt the message using private-key (symmetric) cryptography"),
		OPT_LONG_STRING("password", "password", "provide the password necessary to decrypt the message"),
		OPT_STRING('f', "file", "filename", "specify a file to be encrypted as the message"),
		OPT_END()
};

/* Function Prototypes */
static void show_message_usage(int err, const char *optional_message_format, ...);

/* Public Functions */
int cmd_message(int argc, char *argv[]) {
	show_message_usage(argc, argv[0]);
	return 0;
}

/* Internal Functions */
static void show_message_usage(int err, const char *optional_message_format, ...)
{
	va_list varargs;
	va_start(varargs, optional_message_format);

	variadic_show_usage_with_options(message_cmd_usage, message_cmd_options,
			optional_message_format, varargs, err);

	va_end(varargs);
}
