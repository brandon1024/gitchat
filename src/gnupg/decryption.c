#include <errno.h>

#include "gnupg/decryption.h"

int decrypt_asymmetric_message(struct gc_gpgme_ctx *ctx,
		struct strbuf *ciphertext, struct strbuf *output)
{
	gpgme_error_t err;
	int errsv = errno;
	int ret = 0;

	struct gpgme_data *message_in;
	struct gpgme_data *message_out;
	err = gpgme_data_new_from_mem(&message_in, ciphertext->buff, ciphertext->len, 0);
	if (err)
		GPG_FATAL("unable to create GPGME memory data buffer from ciphertext", err);

	err = gpgme_data_new(&message_out);
	if (err)
		GPG_FATAL("unable to create GPGME data buffer for encrypted ciphertext", err);

	// if decryption failed, we won't die FATAL, we will just notify the caller
	err = gpgme_op_decrypt(ctx->gpgme_ctx, message_in, message_out);
	if (err) {
		LOG_WARN("gpg decryption failed unexpectedly: %d %s\n",
				gpgme_err_code(err), gpgme_strerror(err));

		ret = gpgme_err_code(err) == GPG_ERR_NO_DATA ? 1 : -1;

		goto decrypt_failed;
	}

	// read plaintext into message_out strbuf
	if (gpgme_data_seek(message_out, 0, SEEK_SET) != 0) {
		LOG_ERROR("failed to seek to beginning of gpgme data buffer");
		ret = -1;

		goto decrypt_failed;
	}

	char temporary_buffer[1024];
	ssize_t bytes_read;
	while ((bytes_read = gpgme_data_read(message_out, temporary_buffer, 1024)) > 0)
		strbuf_attach(output, temporary_buffer, bytes_read);

	if (bytes_read < 0) {
		LOG_ERROR("failed to read from gpgme data buffer");

		ret = -1;
		goto decrypt_failed;
	}

	gpgme_data_release(message_in);
	gpgme_data_release(message_out);

	errno = errsv;
	return ret;

decrypt_failed:
	gpgme_data_release(message_in);
	gpgme_data_release(message_out);

	return ret;
}
