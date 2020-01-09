#ifndef GIT_CHAT_ENCRYPTION_H
#define GIT_CHAT_ENCRYPTION_H

#include "gpg-common.h"
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
void symmetric_encrypt_plaintext_message(struct gc_gpgme_ctx *ctx,
		struct strbuf *message, struct strbuf *output, const char *passphrase);

#endif //GIT_CHAT_ENCRYPTION_H
