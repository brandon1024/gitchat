#ifndef GIT_CHAT_FS_UTILS_H
#define GIT_CHAT_FS_UTILS_H

#include "strbuf.h"

/**
 * Copy an existing directory to an alternate location, including any directory
 * structure, file modes, and symbolic links.
 *
 * If copy_dir fails for any reason, the current process will be terminated.
 * */
void copy_dir(char *path_from, char *path_to);

/**
 * Copy a file from the src location to the dest location. The source and destination
 * file paths must be full paths to the file to be copied.
 *
 * The new file will assume the given mode.
 *
 * If copy_file fails for any reason, the current process will be terminated.
 * */
void copy_file(const char *dest, const char *src, int mode);

/**
 * Read a symbolic link for the target path, and store the path in the given str_buf.
 *
 * The 'size' parameter is the stat.st_size from the result of a call to lstat()
 * on the link. This is used to help determine the size of the buffer to use.
 *
 * Returns 0 if the target of the symbolic link was successfully read into the
 * string buffer, and 1 otherwise.
 * */
int get_symlink_target(char *symlink_path, struct strbuf *result, size_t size);

/**
 * Populate an empty string buffer with the current working directory.
 *
 * Returns 0 if the operation was successful. Otherwise, returns 1 and errno is
 * set to indicate the error.
 * */
int get_cwd(struct strbuf *buff);

/**
 * Create a new directory with the given base path and directory name.
 *
 * If safe_create_dir fails for any reason, the current process will be terminated.
 * */
void safe_create_dir(char *base_path, char *dir);

/**
 * Search $PATH for a command.  This emulates the path search that
 * execvp would perform, without actually executing the command so it
 * can be used before fork() to prepare to run a command using
 * execve().
 *
 * The caller should ensure that file contains no directory
 * separators.
 *
 * Returns the path to the command, as found in $PATH or NULL if the
 * command could not be found.  The caller assumes ownership of the memory
 * used to store the resultant path.
 */
char *find_in_path(const char *file);

/**
 * Determine whether a file with the given name and path is executable.
 *
 * Returns 1 if the given file is executable, 0 otherwise.
 * */
int is_executable(const char *name);

#endif //GIT_CHAT_FS_UTILS_H
