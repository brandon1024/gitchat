#include <stdio.h>
#include <stdarg.h>

#include "usage.h"

static const struct usage_description read_cmd_usage[] = {
		USAGE("git chat read [<options>] (<commit-hash> | <filename>)"),
		USAGE("git chat read [(-n | --max-count) <number>] [--dir (ASC | DSC)] [--oneline] [--short]"),
		USAGE("git chat read [--from <email>] [--to <email>]"),
		USAGE("git chat read [--pull-new]"),
		USAGE_END()
};

static const struct option_description read_cmd_options[] = {
		OPT_INT('n', "max-count", "limit number of messages displayed"),
		OPT_LONG_STRING("dir", "direction", "show ascending or descending by date"),
		OPT_LONG_BOOL("oneline", "show each message on single line"),
		OPT_LONG_BOOL("short", "show message in a short format"),
		OPT_LONG_STRING("from", "email", "show messages received from specific user"),
		OPT_LONG_STRING("to", "email", "show messages to a specific user"),
		OPT_LONG_BOOL("pull-new", "pull new messages before displaying"),
		OPT_END()
};

/* Function Prototypes */
static void show_read_usage(int err, const char *optional_message_format, ...);

/* Public Functions */
int cmd_read(int argc, char *argv[])
{
	show_read_usage(0, NULL);
	return 0;
}

/* Internal Functions */
static void show_read_usage(int err, const char *optional_message_format, ...)
{
	va_list varargs;
	va_start(varargs, optional_message_format);

	variadic_show_usage_with_options(read_cmd_usage, read_cmd_options,
			optional_message_format, varargs, err);

	va_end(varargs);
}
