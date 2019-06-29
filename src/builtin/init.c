#include <stdio.h>
#include <stdlib.h>
#include <stdbool.h>
#include <string.h>
#include <stdarg.h>
#include <unistd.h>
#include <limits.h>
#include <errno.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <utils.h>
#include <dirent.h>
#include <fcntl.h>

#include "run-command.h"
#include "usage.h"

/* This is not a POSIX standard, so need to define it if it isn't defined */
#ifndef PATH_MAX
#define PATH_MAX 4096
#endif

#define BUFF_LEN 1024

#ifndef DEFAULT_GIT_CHAT_TEMPLATES_DIR
#define DEFAULT_GIT_CHAT_TEMPLATES_DIR "/usr/local/share/git-chat/templates"
#endif //DEFAULT_GIT_CHAT_TEMPLATES_DIR

static const struct usage_description init_cmd_usage[] = {
		USAGE("git chat init [(-r | --name) <name>] [(-d | --description) <desc>]"),
		USAGE("git chat init [-q | --quiet] [--config <config>]"),
		USAGE("git chat init (-h | --help)"),
		USAGE_END()
};

static const struct option_description init_cmd_options[] = {
		OPT_STRING('n', "name", "name", "Specify a name for the master channel"),
		OPT_STRING('d', "description", "desc", "Specify a description for the space"),
		OPT_BOOL('q', "quiet", "Only print error and warning messages"),
		OPT_BOOL('h', "help", "Show usage and exit"),
		OPT_END()
};

/* Function Prototypes */
static int init(char *room_name, char *space_desc, int quiet);
static void copy_dir(char *path_from, char *path_to);
static int get_symlink_target(char *symlink_path, struct strbuf *result, size_t size);
void copy_file(const char *dest, const char *src, int mode);
static void get_cwd(struct strbuf *buff);
static void safe_create_dir(char *base_path, char *dir);
static void show_init_usage(int err, const char *optional_message_format, ...);

/* Public Functions */
int cmd_init(int argc, char *argv[])
{
	char *room_name = NULL;
	char *room_desc = NULL;
	bool opt_quiet = false;

	for(size_t arg_index = 0; arg_index < argc; arg_index++) {
		char *arg = argv[arg_index];

		if(!is_valid_argument(arg, init_cmd_options)) {
			show_init_usage(1, "error: unknown flag '%s'", arg);
			return 1;
		}

		//room name
		if(argument_matches_option(arg, init_cmd_options[0])) {
			room_name = argv[arg_index];
			continue;
		}

		//room description
		if(argument_matches_option(arg, init_cmd_options[1])) {
			room_desc = argv[arg_index];
			continue;
		}

		//quiet
		if(argument_matches_option(arg, init_cmd_options[3])) {
			opt_quiet = true;
			continue;
		}

		//show help
		if(argument_matches_option(arg, init_cmd_options[4])) {
			show_init_usage(0, NULL);
			return 0;
		}
	}

	return init(room_name, room_desc, opt_quiet);
}

/* Internal Functions */
static int init(char *room_name, char *space_desc, int quiet)
{
	struct child_process_def cmd;
	child_process_def_init(&cmd);

	cmd.git_cmd = 1;
	cmd.no_out = quiet ? 1 : 0;
	cmd.no_err = quiet ? 1 : 0;
	cmd.no_in = 1;

	argv_array_push(&cmd.args, "init", NULL);
	if(run_command(&cmd))
		FATAL("Failed to 'git init' from the current directory.");

	child_process_def_release(&cmd);

	// create .keys and .cache dirs
	struct strbuf cwd;
	strbuf_init(&cwd);
	get_cwd(&cwd);
	safe_create_dir(cwd.buff, ".keys");

	//recursively copy from templates dir into .cache
	strbuf_attach_str(&cwd, "/.cache");
	copy_dir(DEFAULT_GIT_CHAT_TEMPLATES_DIR, cwd.buff);
	strbuf_release(&cwd);

	//todo update config
	//todo set room name and description

	copy_dir("/home/brandon/Downloads/source", "/home/brandon/Downloads/destination");

	return 0;
}

/**
 * Copy an existing directory to an alternate location, including any directory
 * structure, file modes, and symbolic links.
 * */
static void copy_dir(char *path_from, char *path_to)
{
	DIR *dir;
	struct dirent *ent;

	dir = opendir(path_from);
	if (!dir)
		FATAL("Unable to open directory '%s'", path_from);

	safe_create_dir(path_to, NULL);
	while ((ent = readdir(dir)) != NULL) {
		struct stat st_from;

		if (ent->d_name[0] == '.')
			continue;

		struct strbuf new_path_from;
		struct strbuf new_path_to;
		strbuf_init(&new_path_from);
		strbuf_init(&new_path_to);
		strbuf_attach_str(&new_path_from, path_from);
		strbuf_attach_chr(&new_path_from, '/');
		strbuf_attach_str(&new_path_from, ent->d_name);
		strbuf_attach_str(&new_path_to, path_to);
		strbuf_attach_chr(&new_path_to, '/');
		strbuf_attach_str(&new_path_to, ent->d_name);

		if (lstat(new_path_from.buff, &st_from) && errno == ENOENT)
			FATAL("Unable to stat directory '%s'", new_path_from.buff);

		if (S_ISDIR(st_from.st_mode)) {
			//need to recurse.
			copy_dir(new_path_from.buff, new_path_to.buff);
		} else if (S_ISLNK(st_from.st_mode)) {
			//read the target of the symbolic link, then attempt to symlink.
			struct strbuf target;
			strbuf_init(&target);
			if (get_symlink_target(new_path_from.buff, &target, st_from.st_size))
				FATAL("Unable to read symlink target for '%s'", new_path_from.buff);

			if (symlink(target.buff, new_path_to.buff))
				FATAL("Cannot symlink '%s' to '%s'");

			strbuf_release(&target);
		} else if (S_ISREG(st_from.st_mode)) {
			copy_file(new_path_to.buff, new_path_from.buff, st_from.st_mode);
		} else
			DIE("Cannot copy from '%s' to '%s'; unexpected file",
					new_path_from.buff, new_path_to.buff);

		strbuf_release(&new_path_from);
		strbuf_release(&new_path_to);
	}

	closedir(dir);
}

/**
 * Read a symbolic link for the target path, and store the path in the given str_buf.
 *
 * The 'size' parameter is the stat.st_size from the result of a call to lstat()
 * on the link. This is used to help determine the size of the buffer to use.
 * */
static int get_symlink_target(char *symlink_path, struct strbuf *result, size_t size)
{
	if (size < 64)
		size = 64;

	while (size < PATH_MAX) {
		ssize_t len;

		strbuf_grow(result, size);
		len = readlink(symlink_path, result->buff, size);
		if (len < 0)
			return 1;

		if (len < size)
			return 0;

		size *= 2;
	}

	return 1;
}

/**
 * Copy a file from the src location to the dest location. The source and destination
 * file paths must be full paths to the file to be copied.
 *
 * The new file will assume the given mode.
 * */
void copy_file(const char *dest, const char *src, int mode)
{
	int in_fd, out_fd;
	ssize_t bytes_read;
	char buffer[BUFF_LEN];

	if (!(in_fd = open(src, O_RDONLY)))
		FATAL("Failed to open file '%s'", src);

	if (!(out_fd = open(dest, O_WRONLY | O_CREAT | O_EXCL, mode)))
		FATAL("Failed to open file '%s'", dest);

	while ((bytes_read = read(in_fd, buffer, BUFF_LEN)) > 0) {
		if (write(out_fd, buffer, bytes_read) != bytes_read)
			FATAL("Failed to write to file '%s'", dest);
	}

	if (bytes_read == -1)
		FATAL("Unexpected error while reading from file '%s'", src);

	close(in_fd);
	close(out_fd);
}

static void get_cwd(struct strbuf *buff)
{
	char cwd[PATH_MAX];
	if (!getcwd(cwd, PATH_MAX))
		FATAL("Unable to obtain the current working directory from getcwd().");

	strbuf_attach(buff, cwd, PATH_MAX);
}

static void safe_create_dir(char *base_path, char *dir)
{
	struct strbuf buff;
	strbuf_init(&buff);

	strbuf_attach(&buff, base_path, PATH_MAX);

	if (dir) {
		strbuf_attach_chr(&buff, '/');
		strbuf_attach_str(&buff, dir);
	}

	if (mkdir(buff.buff, 0777) < 0) {
		if(errno != EEXIST)
			FATAL("Unable to create directory '%s'.", buff.buff);

		LOG_WARN("Directory '%s' already exists.", buff.buff);
	}

	strbuf_release(&buff);
}

static void show_init_usage(int err, const char *optional_message_format, ...)
{
	va_list varargs;
	va_start(varargs, optional_message_format);

	variadic_show_usage_with_options(init_cmd_usage, init_cmd_options,
			optional_message_format, varargs, err);

	va_end(varargs);
}
