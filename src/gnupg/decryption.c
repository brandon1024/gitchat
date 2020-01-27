#include <errno.h>

#include "decryption.h"

int decrypt_asymmetric_message(struct gc_gpgme_ctx *ctx,
		struct strbuf *ciphertext, struct strbuf *output)
{
	gpgme_error_t err;
	int errsv = errno;

	struct gpgme_data *message_in;
	struct gpgme_data *message_out;
	err = gpgme_data_new_from_mem(&message_in, ciphertext->buff, ciphertext->len, 0);
	if (err)
		GPG_FATAL("unable to create GPGME memory data buffer from ciphertext", err);

	err = gpgme_data_new(&message_out);
	if (err)
		GPG_FATAL("unable to create GPGME data buffer for encrypted ciphertext", err);

	err = gpgme_op_decrypt(ctx->gpgme_ctx, message_in, message_out);
	if (err)
		GPG_FATAL("unable to decrypt ciphertext", err);

	// read plaintext into message_out strbuf
	int ret = gpgme_data_seek(message_out, 0, SEEK_SET);
	if (ret)
		GPG_FATAL("failed to seek to beginning of gpgme data buffer", err);

	char temporary_buffer[1024];
	while ((ret = gpgme_data_read(message_out, temporary_buffer, 1024)) > 0)
		strbuf_attach(output, temporary_buffer, ret);

	if (ret < 0)
		GPG_FATAL("failed to read from gpgme data buffer", err);

	gpgme_data_release(message_in);
	gpgme_data_release(message_out);

	errno = errsv;
	return 0;
}
