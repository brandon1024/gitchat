#ifndef GIT_CHAT_WORKING_TREE_H
#define GIT_CHAT_WORKING_TREE_H

#include "strbuf.h"

/**
 * Determine whether the current working directory is a valid git-chat space.
 *
 * The current working directory is a git-chat space if the following conditions
 * are met:
 * - `.git-chat` exists and is a directory
 * - `.git` exists and is a directory
 * - `git rev-parse --is-inside-work-tree` returns with a zero exit status
 * */
int is_inside_git_chat_space();

/**
 * If the cwd is a git-chat space, get the absolute path to the local GPG
 * home directory, typically localted under .git/.gnupg.
 *
 * Returns 0 if the path was successfully written to the given buffer, and non-zero
 * if the path could not be obtained for some reason.
 * */
int get_gpg_homedir(struct strbuf *path);

/**
 * If the cwd is a git-chat space, get the absolute path to the .keys directory.
 *
 * Returns 0 if the path was successfully written to the given buffer, and non-zero
 * if the path could not be obtained for some reason.
 * */
int get_keys_dir(struct strbuf *path);

#endif //GIT_CHAT_WORKING_TREE_H
