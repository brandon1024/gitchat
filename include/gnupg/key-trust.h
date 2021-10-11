#ifndef GIT_CHAT_INCLUDE_GNUPG_KEY_TRUST_H
#define GIT_CHAT_INCLUDE_GNUPG_KEY_TRUST_H

#include <sys/types.h>

#include "str-array.h"

/**
 * Read the `.trusted-keys` file under `.git/` and append trusted fingerprints
 * to the given `trusted_keys` str_array.
 *
 * Returns the number of fingerprints read from the trusted-keys file, or -1
 * if the file does not exist or could not be read for some reason.
 * */
ssize_t read_trust_list(struct str_array *trusted_keys);

#endif //GIT_CHAT_INCLUDE_GNUPG_KEY_TRUST_H
