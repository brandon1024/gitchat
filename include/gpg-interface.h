#ifndef GIT_CHAT_GPG_INTERFACE_H
#define GIT_CHAT_GPG_INTERFACE_H

#include "strbuf.h"
#include "str-array.h"

/**
 * Locate the gpg executable, and populate the given strbuf with the path
 * to the executable. The executable is located in the following manner:
 *
 * 1. From the 'gpg.program' configuration in the user's gitconfig
 * 2. "gpg", if found on the path
 * 3. "gpg2", if found on the path
 *
 * Returns 0 if the executable was found, 1 otherwise.
 * */
int find_gpg_executable(struct strbuf *path);

/**
 * Reconstruct the git-chat gpg keyring with a set of gpg keys.
 *
 * The existing keyring is removed, and a new one is created from a directory
 * that contains gpg keys.
 *
 * Returns 0 if the keyring was successfully reconstructed, and 1 if an error
 * occurred.
 * */
int rebuild_gpg_keyring(const char *keyring_path, const char *keys_path);

/**
 * Extract all fingerprints from a given keyring.
 *
 * The fingerprints for public keys and any subkeys are added to the str_array.
 *
 * Returns the number of fingerprints added to the str_array.
 * */
int retrieve_fingerprints_from_keyring(const char *keyring_path,
		struct str_array *fingerprints);

#endif //GIT_CHAT_GPG_INTERFACE_H
