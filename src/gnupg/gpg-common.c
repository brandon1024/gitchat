#include <string.h>
#include <fcntl.h>
#include <errno.h>
#include <locale.h>

#include "gnupg/gpg-common.h"
#include "working-tree.h"
#include "fs-utils.h"

static gpgme_passphrase_cb_t pass_loopback_cb = NULL;
static void *pass_loopback_cb_data = NULL;

const char *get_gpgme_library_version()
{
	return gpgme_check_version(NULL);
}

NORETURN void GPG_FATAL(const char *msg, gpgme_error_t err)
{
	FATAL("%s: %s: %s", msg, gpgme_strsource(err), gpgme_strerror(err));
}

void init_gpgme_openpgp_engine(void)
{
	gpgme_error_t err;
	int errsv = errno;

	LOG_INFO("Initializing GPGME with GPGME_PROTOCOL_OpenPGP engine");

	const char *version = gpgme_check_version(NULL);
	LOG_INFO("Using installed GPGME version %s", version);

	setlocale(LC_ALL, "");
	err = gpgme_set_locale(NULL, LC_CTYPE, setlocale(LC_CTYPE, NULL));
	if (err)
		GPG_FATAL("failed to set GPGME locale", err);

#ifdef LC_MESSAGES
	err = gpgme_set_locale (NULL, LC_MESSAGES, setlocale (LC_MESSAGES, NULL));
	if (err)
		GPG_FATAL("failed to set GPGME locale", err);
#endif

	err = gpgme_engine_check_version(GPGME_PROTOCOL_OpenPGP);
	if (err)
		GPG_FATAL("failed to load OpenPGP engine using installed version of GPGME", err);

	struct _gpgme_engine_info *enginfo;
	err = gpgme_get_engine_info(&enginfo);
	if (err)
		GPG_FATAL("failed to load gpgme engine info", err);

	const char *engine_version = enginfo->version;
	const char *protocol_name = (char *) gpgme_get_protocol_name(GPGME_PROTOCOL_OpenPGP);
	const char *engine_executable = enginfo->file_name;
	const char *engine_config_dir = enginfo->home_dir;
	const char *default_config_dir = gpgme_get_dirinfo("homedir");

	if (!engine_version)
		engine_version = "cannot be determined";
	if (!protocol_name)
		protocol_name = "invalid protocol number";
	if (!engine_executable)
		engine_executable = "unknown";
	if (!engine_config_dir)
		engine_config_dir = default_config_dir;
	if (!engine_config_dir)
		engine_config_dir = "unknown";

	LOG_DEBUG("gpgme crypto engine version number: %s", engine_version);
	LOG_DEBUG("gpgme engine protocol: %s", protocol_name);
	LOG_DEBUG("gpgme protocol implementation binary: %s", engine_executable);
	LOG_DEBUG("gpgme config directory: %s", engine_config_dir);

	errno = errsv;
}

void gpgme_context_init(struct gc_gpgme_ctx *ctx, int use_gc_gpg_homedir)
{
	strbuf_init(&ctx->gnupg_homedir);
	if (get_gpg_homedir(&ctx->gnupg_homedir))
		FATAL("local GPG home directory does not exist or cannot be used for some reason");

	gpgme_error_t err = gpgme_new(&ctx->gpgme_ctx);
	if (err)
		GPG_FATAL("failed to create GPGME context", err);

	if (use_gc_gpg_homedir)
		safe_create_dir(ctx->gnupg_homedir.buff, NULL, S_IRUSR | S_IWUSR | S_IXUSR);

	err = gpgme_ctx_set_engine_info(ctx->gpgme_ctx, GPGME_PROTOCOL_OpenPGP, NULL,
			use_gc_gpg_homedir ? ctx->gnupg_homedir.buff : NULL);
	if (err)
		GPG_FATAL("failed to mutate GPGME engine homedir", err);

	gpgme_set_armor(ctx->gpgme_ctx, 1);

	if (pass_loopback_cb) {
		gpgme_set_pinentry_mode(ctx->gpgme_ctx, GPGME_PINENTRY_MODE_LOOPBACK);
		gpgme_set_passphrase_cb(ctx->gpgme_ctx, pass_loopback_cb, pass_loopback_cb_data);
	} else {
		gpgme_set_pinentry_mode(ctx->gpgme_ctx, GPGME_PINENTRY_MODE_DEFAULT);
		gpgme_set_passphrase_cb(ctx->gpgme_ctx, NULL, NULL);
	}
}

void gpgme_context_release(struct gc_gpgme_ctx *ctx)
{
	gpgme_release(ctx->gpgme_ctx);
	strbuf_release(&ctx->gnupg_homedir);
}

void gpgme_configure_passphrase_loopback(gpgme_passphrase_cb_t cb, void *cb_data)
{
	pass_loopback_cb = cb;
	pass_loopback_cb_data = cb_data;
}

void gpgme_context_set_homedir(struct gc_gpgme_ctx *ctx, const char *home_dir)
{
	gpgme_error_t err;
	if (home_dir) {
		strbuf_clear(&ctx->gnupg_homedir);
		strbuf_attach_str(&ctx->gnupg_homedir, home_dir);

		err = gpgme_ctx_set_engine_info(ctx->gpgme_ctx, GPGME_PROTOCOL_OpenPGP, NULL,
				ctx->gnupg_homedir.buff);
	} else {
		err = gpgme_ctx_set_engine_info(ctx->gpgme_ctx, GPGME_PROTOCOL_OpenPGP, NULL,
				NULL);
	}

	if (err)
		GPG_FATAL("failed to mutate GPGME engine homedir", err);
}

gpgme_error_t gpgme_pass_fd_cb(void *hook, const char *uid_hint,
		const char *passphrase_info, int prev_was_bad, int fd)
{
	int pass_fd = *((int *)hook);
	(void)prev_was_bad;

	if (uid_hint)
		LOG_DEBUG("passphrase callback uid hint: %s", uid_hint);
	if (passphrase_info)
		LOG_DEBUG("passphrase info: %s", passphrase_info);

	char buffer[1024];
	ssize_t bytes_read;
	while ((bytes_read = xread(pass_fd, buffer, 1024)) > 0) {
		if (gpgme_io_writen(fd, buffer, bytes_read) < 0)
			return GPG_ERR_USER_2;
	}

	return 0;
}

gpgme_error_t gpgme_pass_file_cb(void *hook, const char *uid_hint,
		const char *passphrase_info, int prev_was_bad, int fd)
{
	const char *pass_file = (const char *)hook;
	(void)prev_was_bad;

	if (uid_hint)
		LOG_DEBUG("passphrase callback uid hint: %s", uid_hint);
	if (passphrase_info)
		LOG_DEBUG("passphrase info: %s", passphrase_info);

	int pass_fd;
	if ((pass_fd = open(pass_file, O_RDONLY)) < 0)
		return GPG_ERR_USER_1;

	char buffer[1024];
	ssize_t bytes_read;
	while ((bytes_read = xread(pass_fd, buffer, 1024)) > 0) {
		if (gpgme_io_writen(fd, buffer, bytes_read) < 0)
			return GPG_ERR_USER_2;
	}

	return 0;
}

gpgme_error_t gpgme_pass_cb(void *hook, const char *uid_hint,
		const char *passphrase_info, int prev_was_bad, int fd)
{
	const char *pass = (const char *)hook;
	(void)prev_was_bad;

	if (uid_hint)
		LOG_DEBUG("passphrase callback uid hint: %s", uid_hint);
	if (passphrase_info)
		LOG_DEBUG("passphrase info: %s", passphrase_info);

	if (gpgme_io_writen(fd, pass, strlen(pass)) < 0)
		return GPG_ERR_USER_2;
	if (gpgme_io_writen(fd, "\n", 1) < 0)
		return GPG_ERR_USER_2;

	return 0;
}
