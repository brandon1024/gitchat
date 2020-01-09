#ifndef GIT_CHAT_KEY_MANAGER_H
#define GIT_CHAT_KEY_MANAGER_H

#include "gpg-common.h"

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
