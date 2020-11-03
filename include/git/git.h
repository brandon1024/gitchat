#ifndef GIT_CHAT_GIT_H
#define GIT_CHAT_GIT_H

#include <inttypes.h>

#include "str-array.h"
#include "strbuf.h"

#define GIT_RAW_OBJECT_ID 20
#define GIT_HEX_OBJECT_ID 40

/** Unique identity of any object (commit, tree, blob, tag). */
struct git_oid {
	unsigned char id[GIT_RAW_OBJECT_ID];
};

/**
 * Parse a hex-formatted object id into a raw object id. String argument must
 * point to the beginning of a hex sequence with a length of 40 bytes.
 * */
void git_str_to_oid(struct git_oid *oid, const char *str);

/**
 * Format a git_oid as a 40-byte hex string.
 * */
void git_oid_to_str(struct git_oid *oid, char hex_buffer[GIT_HEX_OBJECT_ID]);

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
