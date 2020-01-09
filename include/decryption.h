#ifndef GIT_CHAT_DECRYPTION_H
#define GIT_CHAT_DECRYPTION_H

#include "gpg-common.h"
#include "strbuf.h"

/**
 * Decrypt ascii-armored ciphertext into a given output buffer.
 * */
int decrypt_asymmetric_message(struct gc_gpgme_ctx *ctx,
		struct strbuf *ciphertext, struct strbuf *output);

#endif //GIT_CHAT_DECRYPTION_H
