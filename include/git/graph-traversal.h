#ifndef GIT_CHAT_INCLUDE_GIT_GRAPH_TRAVERSAL_H
#define GIT_CHAT_INCLUDE_GIT_GRAPH_TRAVERSAL_H

#include "git/commit.h"

typedef int (*graph_traversal_cb)(struct git_commit *commit, void *data);

/**
 * Traverse the git commit graph in reverse chronological order, starting at
 * `commit` (which can be a reference or a commit hash) and reading up to
 * `limit` commits. Commit objects are parsed and passed to the callback
 * function `cb`, along with an arbitrary pointer `data`.
 *
 * When `commit` is null, traversal starts from the current commit (HEAD).
 * If `limit` is negative, traverse all commits.
 *
 * Returns zero if the traversal successful, return non-negative if the
 * graph traversal callback returned non-zero, and return negative if an error
 * occurred.
 */
int traverse_commit_graph(const char *commit, int limit, graph_traversal_cb cb,
		void *data);

#endif //GIT_CHAT_INCLUDE_GIT_GRAPH_TRAVERSAL_H
