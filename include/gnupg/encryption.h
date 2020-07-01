#ifndef GIT_CHAT_ENCRYPTION_H
#define GIT_CHAT_ENCRYPTION_H

#include "gnupg/gpg-common.h"
#include "strbuf.h"

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
void asymmetric_encrypt_plaintext_message(struct gc_gpgme_ctx *ctx,
		const struct strbuf *message, struct strbuf *output,
		struct gpg_key_list *recipients);

#endif //GIT_CHAT_ENCRYPTION_H
