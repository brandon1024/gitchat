#ifndef GIT_CHAT_PARSE_OPTIONS_H
#define GIT_CHAT_PARSE_OPTIONS_H

#include <stdarg.h>
#include <stddef.h>

#include "str-array.h"

/**
 * parse-options api
 *
 * The parse-options api provides a standardized interface for interpreting
 * command-line options and providing usage information to the user.
 *
 * Options are represented as an array of `command_option` structs which
 * describe each option, like the option type (boolean, integer, string, etc.)
 * and a pointer to the location in memory where the value of the option is
 * set if that option is provided. This array of command_options is supplied
 * to the `parse_options` and `show_usage` functions.
 *
 * Usage strings are used to provide a simple synopsis of the command to the
 * user, to help describe how the command can be used.
 *
 *
 * Defining Command Options:
 * Options can have any of the following types:
 * - boolean
 *   - short or long format, value of type `int`
 *   - e.g. `$ my-command -b --boolean`
 * - integer
 *   - short or long format, value of type `int`
 *   - e.g. `$ my-command -n 15 -c1 --count=105 --index=0x45`
 * - string
 *   - short or long format, value of type `const char *`
 *   - e.g. `$ my-command -s "my string" --string="my string" --key value`
 * - string list
 *   - short or long format, value of type `struct str_array *`
 *   - e.g. `$ my-command -f file1 -f file2 --file file3`
 *
 * In addition, the following pseudo-options are defined:
 * - command
 *   - a subcommand
 *   - e.g. `$ my-command subcommand`
 * - groups,
 *   - used to group similar options together in usage output
 * - end
 *   - used to terminate a list of options
 *
 * Option macros can be used to easily define the list of options recognized
 * by a program or builtin, as shown in the example below:
 *
 *     char *gpg_home_dir = NULL;
 *     struct str_array key_paths;
 *     int show_help = 0;
 *
 *     const struct command_option options[] = {
 *         OPT_LONG_STRING("gpg-home", "path", "path to the gpg home directory", &gpg_home_dir),
 *         OPT_STRING_LIST('f', "file", "path", "path to exported public key file", &key_paths),
 *         OPT_BOOL('h', "help", "show usage and exit", &show_help),
 *         OPT_END()
 *     };
 *
 * Defining Usage Information:
 * Usage strings help describe to the user what features your command or builtin
 * supports. The syntax is quite simple, as described by the following in
 * Backus-Naur Form:
 *
 *     <usage>            ::= <command> <subcommand> <options> <arguments>
 *     <command>          ::= <string> | <command> " " <string>
 *     <subcommand>       ::= " " <string> | " (" <string> " | " <string> ")" | E
 *     <options>          ::= " " <singleoption> <options> | E
 *     <arguments>        ::= " " <args> | " -- " <args> | " [--] " <args> | E
 *
 *     <singleoption>     ::= <option> | "[" <option> "]"
 *     <option>           ::= <booleanoption> | <integeroption> | <stringoption> | <stringlistoption>
 *     <args>             ::= <optionvalue> | "[" <optionvalue> "]" | <optionvalue> | <args> <optionvalue> | <args> "..."
 *
 *     <shortoption>      ::= "-" <letter>
 *     <longoption>       ::= "--" <string>
 *     <optionvalue>      ::= "<" <string> ">"
 *
 *     <booleanoption>    ::= <shortoption> | <longoption> | "(" <shortoption> " | " <longoption> ")"
 *     <integeroption>    ::= <shortoption> " " <optionvalue> | <longoption> " " <optionvalue> | "(" <shortoption> " | " <longoption> ") " <optionvalue>
 *     <stringoption>     ::= <shortoption> " " <optionvalue> | <longoption> " " <optionvalue> | "(" <shortoption> " | " <longoption> ") " <optionvalue>
 *     <stringlistoption> ::= <shortoption> " " <optionvalue> "..." | <longoption> " " <optionvalue> "..." | "(" <shortoption> " | " <longoption> ") " <optionvalue> "..."
 *
 *     <string>           ::= <string> <letter> | <string> "-" <letter> | <letter>
 *     <letter>           ::= [a-z]
 */

enum opt_type {
	OPTION_BOOL_T,
	OPTION_INT_T,
	OPTION_STRING_T,
	OPTION_STRING_LIST_T,
	OPTION_COMMAND_T,
	OPTION_GROUP_T,
	OPTION_END
};

union opt_arg_value_ptr {
	int *int_ptr;
	char **str_ptr;
	struct str_array *str_array_ptr;
};

struct command_option;
struct command_option {
	const char s_flag;
	const char *l_flag;
	const char *str_name;
	const char *desc;
	enum opt_type type;
	union opt_arg_value_ptr arg_value;
};

struct usage_string;
struct usage_string {
	const char *usage_desc;
};

#define OPT_SHORT_BOOL(S,D,V)			{ (S), NULL,  NULL, (D), OPTION_BOOL_T, { .int_ptr = (V) } }
#define OPT_SHORT_INT(S,D,V)			{ (S), NULL,  NULL, (D), OPTION_INT_T, { .int_ptr = (V) } }
#define OPT_SHORT_STRING(S,N,D,V)		{ (S), NULL,  (N), (D), OPTION_STRING_T, { .str_ptr = (V) } }
#define OPT_SHORT_STRING_LIST(S,N,D,V)		{ (S), NULL,  (N), (D), OPTION_STRING_LIST_T, { .str_array_ptr = (V) } }
#define OPT_LONG_BOOL(L,D,V)			{ 0, (L),  NULL, (D), OPTION_BOOL_T, { .int_ptr = (V) } }
#define OPT_LONG_INT(L,D,V)				{ 0, (L),  NULL, (D), OPTION_INT_T, { .int_ptr = (V) } }
#define OPT_LONG_STRING(L,N,D,V)		{ 0, (L),  (N), (D), OPTION_STRING_T, { .str_ptr = (V) } }
#define OPT_LONG_STRING_LIST(L,N,D,V)		{ 0, (L),  (N), (D), OPTION_STRING_LIST_T, { .str_array_ptr = (V) } }
#define OPT_BOOL(S,L,D,V)				{ (S), (L), NULL, (D), OPTION_BOOL_T, { .int_ptr = (V) } }
#define OPT_INT(S,L,D,V)				{ (S), (L), NULL, (D), OPTION_INT_T, { .int_ptr = (V) } }
#define OPT_STRING(S,L,N,D,V)			{ (S), (L), (N), (D), OPTION_STRING_T, { .str_ptr = (V) } }
#define OPT_STRING_LIST(S,L,N,D,V)			{ (S), (L), (N), (D), OPTION_STRING_LIST_T, { .str_array_ptr = (V) } }
#define OPT_CMD(N,D,V)					{ 0, NULL, (N), (D), OPTION_COMMAND_T, { .str_ptr = (V) } }
#define OPT_GROUP(N)					{ 0, NULL, NULL, N, OPTION_GROUP_T }
#define OPT_END()						{ 0, NULL, NULL, NULL, OPTION_END }

#define USAGE(DESC)					{ (DESC) }
#define USAGE_END()					{ NULL }


/**
 * Parse command line arguments against an array of command_option's which describe
 * acceptable valid arguments.
 *
 * If an argument in the argument vector matches a command_option, the argument
 * (along with any applicable arg values) are shifted from the argument vector.
 *
 * If `--` is encountered, processing stops and all remaining args are left in the
 * argument vector.
 *
 * If skip_first is non-zero, the first argument in argv is skipped. This is useful
 * when processing arguments with the first argument being the program name.
 *
 * If stop_on_unknown is non-zero, all arguments are parsed even if the argument
 * is not recognized.
 *
 * Returns the number of arguments left in the argv array.
 * */
int parse_options(int argc, char *argv[], const struct command_option options[],
		int skip_first, int stop_on_unknown);

/**
 * Print usage of a command by supplying a list of usage_strings. Provide
 * an optional format string to display in the case of an error, along with the
 * associated arguments.
 *
 * If err is non-zero, outputs to stderr. Otherwise, outputs to stdout.
 *
 * See stdio printf() for format string specification
 * */
void show_usage(const struct usage_string cmd_usage[], int err,
		const char *optional_message_format, ...);

/**
 * Variadic form of show_usage, allowing the use of va_list rather than
 * arbitrary arguments.
 * */
void variadic_show_usage(const struct usage_string cmd_usage[],
		const char *optional_message_format, va_list varargs, int err);

/**
 * Print usage of a command to stdout by supplying a list of usage_descriptions.
 *
 * If err is non-zero, outputs to stderr. Otherwise, outputs to stdout.
 * */
void show_options(const struct command_option opts[], int err);

/**
 * Combination of show_usage() and show_options().
 * */
void show_usage_with_options(const struct usage_string cmd_usage[],
		const struct command_option opts[], int err,
		const char *optional_message_format, ...);

/**
 * Variadic form of show_usage_with_options, allowing the use of va_list rather
 * than arbitrary arguments.
 * */
void variadic_show_usage_with_options(const struct usage_string cmd_usage[],
		const struct command_option opts[],
		const char *optional_message_format, va_list varargs, int err);

#endif //GIT_CHAT_PARSE_OPTIONS_H
