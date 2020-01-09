#ifndef GIT_CHAT_KEY_FILTER_H
#define GIT_CHAT_KEY_FILTER_H

#include "gpg-common.h"

/**
 * Filter a gpg_key_list according to the return value of a filter predicate
 * function.
 *
 * The filter predicate function accepts a gpg key structure and an optional
 * pointer to some arbitrary data and returns 0 if the key is should be filtered
 * from the list, or 1 if the key is left.
 *
 * Filtered list nodes are released, along with the gpg key data stored within.
 *
 * Returns the number of keys (list nodes) filtered.
 * */
int filter_gpg_keys_by_predicate(struct gpg_key_list *keys,
		int (*predicate)(struct _gpgme_key *key, void *data), void *optional_data);

/**
 * Predefined filter function which can be used to filter gpg keys that are either:
 * - expired,
 * - disabled,
 * - invalid,
 * - or cannot be used for encryption.
 * */
int filter_gpg_unusable_keys(struct _gpgme_key *key, void *data);

/**
 * Predefined filter function used to filter secret keys from a key list.
 * */
int filter_gpg_secret_keys(struct _gpgme_key *key, void *data);

#endif //GIT_CHAT_KEY_FILTER_H
