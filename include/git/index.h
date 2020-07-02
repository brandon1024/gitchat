#ifndef GIT_CHAT_INDEX_H
#define GIT_CHAT_INDEX_H

#include "str-array.h"

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

#endif //GIT_CHAT_INDEX_H
