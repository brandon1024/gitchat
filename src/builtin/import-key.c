#include <fcntl.h>

#include "parse-options.h"
#include "gpg-common.h"
#include "fs-utils.h"
#include "working-tree.h"
#include "utils.h"

static const struct usage_string import_key_cmd_usage[] = {
		USAGE("git chat import-key [--keyring <path>] --keyid <id>"),
		USAGE("git chat import-key ((-k | --key) <path>)"),
		USAGE("git chat import-key (-h | --help)"),
		USAGE_END()
};

static int import_key_from_keyring(const char *, const char *);
static int import_key_from_file(const char *);

int cmd_import_key(int argc, char *argv[])
{
	char *keyring_path = NULL;
	char *key_id = NULL;
	char *key_path = NULL;
	int show_help = 0;

	const struct command_option options[] = {
			OPT_LONG_STRING("keyring", "path", "path to the keyring", &keyring_path),
			OPT_LONG_STRING("keyid", "id", "id of the public key to import", &key_id),
			OPT_STRING('k', "key", "path", "path to exported public key file", &key_path),
			OPT_BOOL('h', "help", "show usage and exit", &show_help),
			OPT_END()
	};

	argc = parse_options(argc, argv, options, 1, 1);
	if (argc) {
		show_usage_with_options(import_key_cmd_usage, options, 1, "unknown option '%s'", argv[0]);
		return 1;
	}

	if (show_help) {
		show_usage_with_options(import_key_cmd_usage, options, 0, NULL);
		return 0;
	}

	if (key_id) {
		if (key_path) {
			show_usage_with_options(import_key_cmd_usage, options, 1, "cannot combine --keyring and --key options");
			return 1;
		}

		return import_key_from_keyring(key_id, keyring_path);
	}

	if (key_path) {
		if (keyring_path) {
			show_usage_with_options(import_key_cmd_usage, options, 1, "cannot combine --key and --keyring options");
			return 1;
		}

		if (key_id) {
			show_usage_with_options(import_key_cmd_usage, options, 1, "cannot combine --key and --keyid options");
			return 1;
		}

		return import_key_from_file(key_path);
	}

	show_usage_with_options(import_key_cmd_usage, options, 1, "nothing to do");
	return 1;
}

static int import_key_from_keyring(const char *key_id, const char *optional_keyring_path)
{
	if (!is_inside_git_chat_space())
		DIE("Where are you? It doesn't look like you're in the right directory.");

	(void) key_id;
	(void) optional_keyring_path;

	return 1;
}

static int import_key_from_file(const char *file_path)
{
	if (!is_inside_git_chat_space())
		DIE("Where are you? It doesn't look like you're in the right directory.");

	struct gc_gpgme_ctx ctx;
	gpgme_context_init(&ctx, 1);

	struct gpgme_data *key_data;
	gpgme_error_t err = gpgme_data_new_from_file(&key_data, file_path, 1);
	if (err)
		FATAL("Failed to read key file '%s'", file_path);

	err = gpgme_op_import(ctx.gpgme_ctx, key_data);
	switch(err) {
		case GPG_ERR_INV_VALUE:
			BUG("failed to import key from file");
		case GPG_ERR_NO_DATA:
			BUG("failed to import key from file; file or buffer was empty");
		case GPG_ERR_NO_ERROR:
		default:
			LOG_DEBUG("Successfully imported key from file");
	}

	gpgme_data_release(key_data);

	struct strbuf keys_dir;
	strbuf_init(&keys_dir);
	if (get_keys_dir(&keys_dir))
		FATAL("failed to obtain git-chat keys dir");

	struct str_array key_fingerprints;
	str_array_init(&key_fingerprints);

	gpgme_import_result_t import_result = gpgme_op_import_result(ctx.gpgme_ctx);
	gpgme_import_status_t result = import_result->imports;
	while (result) {
		if (result->result != GPG_ERR_NO_ERROR)
			DIE("failed to import GPG key");
		if ((result->status & GPGME_IMPORT_SECRET) == GPGME_IMPORT_SECRET) {
			// fixme: we need a better approach here, since this won't completely rebuild the keyring
			rebuild_gpg_keyring(&ctx, keys_dir.buff);
			DIE("you accidentally imported a private key, which is insecure so we removed it.");
		}

		LOG_DEBUG("imported key with fingerprint %s", result->fpr);
		str_array_push(&key_fingerprints, result->fpr, NULL);

		result = result->next;
	}

	gpg_context_release(&ctx);

	// then, export the key into the keys directory
	gpgme_context_init(&ctx, 1);
	gpgme_set_armor(ctx.gpgme_ctx, 1);

	for (size_t i = 0; i < key_fingerprints.len; i++) {
		char *fpr = str_array_get(&key_fingerprints, i);
		err = gpgme_data_new(&key_data);
		// TODO check err
		err = gpgme_op_export(ctx.gpgme_ctx, fpr, 0, key_data);
		// TODO check err

		struct strbuf key_path;
		strbuf_init(&key_path);
		strbuf_attach_fmt(&key_path, "%s/%s", keys_dir.buff, fpr);

		int fd = open(key_path.buff, O_WRONLY | O_CREAT | O_EXCL,
				S_IRUSR | S_IWUSR | S_IRGRP | S_IROTH);
		if (fd < 0)
			FATAL(FILE_OPEN_FAILED, key_path.buff);

		// TODO write the key to the file
		ssize_t bytes_read;
		unsigned char buffer[1024];
		while ((bytes_read = gpgme_data_read(key_data, buffer, 1024))) {
			if (recoverable_write(fd, buffer, bytes_read) != bytes_read)
				FATAL("file write failed");
		}

		strbuf_release(&key_path);
	}

	gpg_context_release(&ctx);
	strbuf_release(&keys_dir);
	str_array_release(&key_fingerprints);

	return 0;
}
