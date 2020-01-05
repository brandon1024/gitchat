#ifndef GIT_CHAT_GPG_INTERFACE_H
#define GIT_CHAT_GPG_INTERFACE_H

#include <gpgme.h>

#include "strbuf.h"
#include "str-array.h"

struct gpg_key_list {
	struct gpg_key_list_node *head;
	struct gpg_key_list_node *tail;
};

struct gpg_key_list_node {
	struct _gpgme_key *key;
	struct gpg_key_list_node *next;
	struct gpg_key_list_node *prev;
};


/**
 * Initialize the gpgme OpenPGP engine.
 *
 * The gpgme engine is initialized under the GPGME_PROTOCOL_OpenPGP protocol.
 *
 * The gpgme engine is initialized using the default gpg executable and configuration
 * directory. To use an alternate gpg homedir, set the homedir on newly created
 * gpgme contexts.
 * */
void init_gpgme_openpgp_engine(void);

/**
 * Initialize a gpgme_context, with an optional new GNUPGHOME directory.
 * */
void gpgme_context_init(struct gpgme_context **ctx, char *new_homedir);

/**
 * Release any resources under a gpgme_context.
 * */
void gpg_context_release(struct gpgme_context **ctx);

/**
 * Encrypt a plaintext message in ASCII-armor format into a given string buffer.
 * The message is encrypted using asymmetric (public key) encryption.
 *
 * The message in the `message strbuf is encrypted for all recipients in the
 * recipients key list. `recipients` must not be empty.
 *
 * The message is encrypted with the following gpgme encrypt flags:
 * - GPGME_ENCRYPT_ALWAYS_TRUST
 * - GPGME_ENCRYPT_NO_ENCRYPT_TO
 * */
void asymmetric_encrypt_plaintext_message(struct gpgme_context *ctx,
		const struct strbuf *message, struct strbuf *output,
		struct gpg_key_list *recipients);

/**
 * Encrypt a plaintext message in ASCII-armor format into a given string buffer.
 * The message is encrypted using symmetric (password) encryption.
 *
 * If passphrase is NULL, the default pinentry method is used. This functionality
 * is provided by the gpgme library, and is the preferred method.
 *
 * If passphrase is non-NULL, the passphrase is provided via a gpgme passphrase
 * callback. Note that for gpg versions 2.1.0 - 2.1.12 this mode requires
 * allow-loopback-pinentry to be enabled in the gpg-agent.conf or an agent
 * started with that option.
 * */
void symmetric_encrypt_plaintext_message(struct gpgme_context *ctx,
		struct strbuf *message, struct strbuf *output, const char *passphrase);

/**
 * TODO DOCUMENT ME
 * */
int decrypt_asymmetric_message(struct gpgme_context *ctx,
		struct strbuf *ciphertext, struct strbuf *output);

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
int rebuild_gpg_keyring(struct gpgme_context *ctx, const char *keys_dir);

/**
 * Build a linked list of gpg keys from the keyring under the gpg homedir given.
 *
 * The given gpg_key_list must be empty.
 *
 * Returns the number of keys inserted into the gpg_key_list.
 * */
int get_gpg_keys_from_keyring(struct gpgme_context *ctx, struct gpg_key_list *keys);

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

/**
 * Release any resources under a gpg_key_list, including any gpg key data.
 *
 * The gpg_key_list is reinitialized.
 * */
void release_gpg_key_list(struct gpg_key_list *keys);

/**
 * Retrieve a statically-allocated string representing the version of the gpgme
 * library.
 * */
const char *get_gpgme_library_version();

#endif //GIT_CHAT_GPG_INTERFACE_H
