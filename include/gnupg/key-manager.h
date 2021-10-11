#ifndef GIT_CHAT_KEY_MANAGER_H
#define GIT_CHAT_KEY_MANAGER_H

#include "gnupg/gpg-common.h"
#include "str-array.h"

/**
 * Import an ASCII-armored gpg key from a file into the configured gpg keyring.
 *
 * imported_key_fingerprints is updated with the fingerprints of all
 * imported keys.
 *
 * Returns the number of keys successfully imported.
 * */
int import_gpg_key(struct gc_gpgme_ctx *ctx, const char *key_file_path,
		struct str_array *imported_key_fingerprints);

/**
 * From the configured gpg keyring, export a key with a given fingerprint to
 * a given file.
 *
 * Returns zero if the key was exported successfully, or 1 if a gpgme error
 * occurred, and -1 if unable to create the exported key file.
 * */
int export_gpg_key(struct gc_gpgme_ctx *ctx, const char *fingerprint,
		const char *file_path);

/**
 * Reimport any gpg keys from the given keys directory into the gpg keyring.
 *
 * gpg_homedir must be the full path to the gpg home directory, typically
 * found in `<repository>/.git/.gnupg`. The config directory will be created if
 * it does not exist.
 *
 * Returns the number of keys that were imported. If a key failed to be imported,
 * the application will DIE().
 * */
int rebuild_gpg_keyring(struct gc_gpgme_ctx *ctx, const char *keys_dir);

/**
 * Build a linked list of all gpg keys from the (git-chat) keyring. Returns any
 * and all keys, including any private keys that might exist in the keyring.
 *
 * The given gpg_key_list must be empty. Returns the number of keys inserted
 * into the gpg_key_list.
 * */
int fetch_gpg_keys(struct gc_gpgme_ctx *ctx, struct gpg_key_list *keys);

/**
 * Release any resources under a gpg_key_list, including any gpg key data.
 *
 * The gpg_key_list is reinitialized.
 * */
void release_gpg_key_list(struct gpg_key_list *keys);

#endif //GIT_CHAT_KEY_MANAGER_H
