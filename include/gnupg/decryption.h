#ifndef GIT_CHAT_DECRYPTION_H
#define GIT_CHAT_DECRYPTION_H

#include "gnupg/gpg-common.h"
#include "strbuf.h"

/**
 * Decrypt ascii-armored ciphertext into a given output buffer.
 *
 * Returns zero if message decrypted successfully, > 0 if no data to decrypt
 * or < 0 if decryption failed for any other reason.
 * */
int decrypt_asymmetric_message(struct gc_gpgme_ctx *ctx,
		struct strbuf *ciphertext, struct strbuf *output);

#endif //GIT_CHAT_DECRYPTION_H
