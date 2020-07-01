#include <stdlib.h>
#include <string.h>
#include <errno.h>

#include "gnupg/encryption.h"

void asymmetric_encrypt_plaintext_message(struct gc_gpgme_ctx *ctx,
		const struct strbuf *message, struct strbuf *output,
		struct gpg_key_list *recipients)
{
	gpgme_error_t err;
	int errsv = errno;

	LOG_INFO("Encrypting plaintext message");

	struct str_array keys;
	str_array_init(&keys);

	// use str_array to build fixed-length array from linked list of gpg keys
	struct gpg_key_list_node *node = recipients->head;
	while (node) {
		struct str_array_entry *entry = str_array_insert_nodup(&keys, NULL, keys.len);
		entry->data = node->key;

		LOG_TRACE("Recipient gpg key fingerprint: %s", node->key->fpr);

		node = node->next;
	}

	size_t keys_len = 0;
	struct _gpgme_key **keys_array = (struct _gpgme_key **)str_array_detach_data(&keys, &keys_len);
	if (!keys_len)
		BUG("no gpg keys given to asymmetric_encrypt_plaintext_message()");

	// build gpg data buffers for the plaintext input and ciphertext output
	struct gpgme_data *message_in;
	struct gpgme_data *message_out;
	err = gpgme_data_new_from_mem(&message_in, message->buff, message->len, 0);
	if (err)
		GPG_FATAL("unable to create GPGME memory data buffer from plaintext message", err);

	err = gpgme_data_new(&message_out);
	if (err)
		GPG_FATAL("unable to create GPGME data buffer for encrypted ciphertext", err);

	// encrypt plaintext, always trusting gpg keys, and do not use default recipient
	err = gpgme_op_encrypt(ctx->gpgme_ctx, keys_array, GPGME_ENCRYPT_ALWAYS_TRUST | GPGME_ENCRYPT_NO_ENCRYPT_TO,
			message_in, message_out);
	if (err) {
		if (gpgme_err_code(err) == GPG_ERR_INV_VALUE)
			BUG("invalid pointer passed to gpgme_op_encrypt(...)");

		if (gpgme_err_code(err) == GPG_ERR_UNUSABLE_PUBKEY) {
			struct _gpgme_op_encrypt_result *result = gpgme_op_encrypt_result(ctx->gpgme_ctx);

			fprintf(stderr, "cannot encrypt message with invalid recipient:\n");

			struct _gpgme_invalid_key *invalid_key = result->invalid_recipients;
			while (invalid_key) {
				fprintf(stderr, "\t%s - Reason: %s\n", invalid_key->fpr, gpgme_strerror(invalid_key->reason));

				invalid_key = invalid_key->next;
			}
		}

		GPG_FATAL("GPGME unable to encrypt message", err);
	}

	// read ciphertext into message_out strbuf
	int ret = gpgme_data_seek(message_out, 0, SEEK_SET);
	if (ret)
		GPG_FATAL("failed to seek to beginning of gpgme data buffer", err);

	char temporary_buffer[1024];
	while ((ret = gpgme_data_read(message_out, temporary_buffer, 1024)) > 0)
		strbuf_attach(output, temporary_buffer, ret);

	if (ret < 0)
		GPG_FATAL("failed to read from gpgme data buffer", err);

	// free the array but leave the gpg keys intact; they are owned by the caller
	free(keys_array);

	gpgme_data_release(message_in);
	gpgme_data_release(message_out);

	LOG_INFO("Successfully encrypted message");
	errno = errsv;
}
