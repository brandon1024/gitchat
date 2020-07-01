#ifndef GIT_CHAT_GIT_H
#define GIT_CHAT_GIT_H

#include "str-array.h"
#include "strbuf.h"

/**
 * Add a single file to the git index. This is equivalent to running the following:
 * git add <file>
 * */
int git_add_file_to_index(const char *file);

/**
 * Add files to the git index from an array of strings. This is equivalent to
 * running the following:
 * git add <file>...
 * */
int git_add_files_to_index(struct str_array *file_paths);

/**
 * Commit to the git tree any files currently tracked under the git index.
 * The commit will have the given commit message. This is equivalent to running
 * the following:
 * git commit -m <message>
 * */
int git_commit_index(const char *commit_message);

/**
 * Similar to git_commit_index(), create a new commit with a given commit message
 * and with optional command-line arguments.
 *
 * This is equivalent to running the following:
 * git commit -m <message> <args>...
 * */
__attribute__ ((sentinel))
int git_commit_index_with_options(const char *commit_message, ...);

/**
 * Attempt to fetch the user identify from their .gitconfig. The author's name
 * is chosen, in the following order:
 * 1. user.username
 * 2. user.email
 * 3. user.name
 *
 * If none of these are available (i.e. git returns a status of 1 for all three),
 * then this function returns 1. Otherwise, returns 0 and populates the given
 * strbuf with the author's name.
 * */
int get_author_identity(struct strbuf *result);

#endif //GIT_CHAT_GIT_H
