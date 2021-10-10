#include <string.h>
#include <fcntl.h>

#include "parse-options.h"
#include "gnupg/gpg-common.h"
#include "git/git.h"
#include "git/index.h"
#include "git/commit.h"
#include "working-tree.h"
#include "utils.h"
#include "fs-utils.h"

static const struct usage_string import_key_cmd_usage[] = {
		USAGE("git chat import-key [--gpg-home <path>] [((-f | --file) <path>)...] [--] <key fpr>..."),
		USAGE("git chat import-key (-h | --help)"),
		USAGE_END()
};

/**
 * Predicate that filters keys that are unusable (expired, private, revoked, etc.),
 * and whose fingerprint matches an entry in the str_array provided through the
 * `data` argument.
 *
 * If `data` is null, the fingerprint check is skipped.
 *
 * Returns 1 if the key is usable, otherwise returns 0.
 * */
static int gpg_keylist_filter_predicate(gpgme_key_t key, void *data)
{
	struct str_array *fingerprints = (struct str_array *) data;
	struct str_array_entry *entry = NULL;

	// lookup the str_array entry with a matching fingerprint
	for (size_t i = 0; i < fingerprints->len; i++) {
		struct str_array_entry *current = str_array_get_entry(fingerprints, i);

		if (!strcmp(key->fpr, current->string)) {
			entry = current;
			break;
		}
	}

	// filter keys by fingerprint
	if (!entry)
		return 0;

	// filter secret keys
	if (!filter_gpg_secret_keys(key, NULL))
		return 0;

	// filter unusable keys
	if (!filter_gpg_unusable_keys(key, NULL)) {
		WARN("key with fingerprint '%s' is unusable; the key may be expired, disabled, removed or invalid",
				key->fpr);
		return 0;
	}

	return 1;
}

/**
 * Commit the contents of the index, with a plain commit message indicating
 * the author of the message and the owner of the keys being committed.
 * */
static void commit_keys_from_index(struct str_array *key_fingerprints)
{
	struct strbuf message;
	strbuf_init(&message);

	if (get_author_identity(&message))
		strbuf_attach_str(&message, "unknown user");

	strbuf_attach_str(&message, " joined the channel.\n");

	for (size_t fpr_index = 0; fpr_index < key_fingerprints->len; fpr_index++)
		strbuf_attach_fmt(&message, "\n\tkey [fpr: %s]",
				str_array_get(key_fingerprints, fpr_index));

	if (git_commit_index_with_options(message.buff, "--no-gpg-sign",
			"--no-verify", NULL))
		DIE("unable to commit exported gpg key(s) to tree");

	strbuf_release(&message);
}

/**
 * Import one or more GPG keys from a list of file paths into the git-chat GPG
 * keyring. The fingerprints of keys successfully imported are added to
 * the `imported_key_fprs` str_array.
 *
 * Returns zero if successful, and non-zero otherwise.
 * */
static int import_keys_from_files(struct str_array *key_paths,
		struct str_array *imported_key_fprs)
{
	struct gc_gpgme_ctx gc_ctx;
	struct gpg_key_list gpg_keys;
	struct str_array key_fprs;

	if (!key_paths->len)
		return 0;

	gpgme_context_init(&gc_ctx, 1);

	str_array_init(&key_fprs);

	// import the keys into into the git-chat keyring
	for (size_t f_index = 0; f_index < key_paths->len; f_index++) {
		char *file_path = str_array_get(key_paths, f_index);

		// import key from file into git-chat keyring
		int key_count = import_gpg_key(&gc_ctx, file_path, &key_fprs);
		if (!key_count)
			WARN("unable to import key from file '%s'", file_path);
	}

	// list and filter unusable keys
	fetch_gpg_keys(&gc_ctx, &gpg_keys);
	filter_gpg_keys_by_predicate(&gpg_keys, gpg_keylist_filter_predicate,
			(void *) &key_fprs);

	struct gpg_key_list_node *key = gpg_keys.head;
	while (key) {
		str_array_push(imported_key_fprs, key->key->fpr, NULL);
		key = key->next;
	}

	str_array_release(&key_fprs);

	release_gpg_key_list(&gpg_keys);
	gpgme_context_release(&gc_ctx);

	return 0;
}

/**
 * Import one or more GPG keys from a list of key fingerprints into the git-chat
 * GPG keyring. The fingerprints of keys successfully imported are added to
 * the `imported_key_fprs` str_array.
 *
 * If `optional_gpg_home` is non-null, keys are exported from that GPG home
 * directory instead of the default one.
 *
 * Returns zero if successful, and non-zero otherwise.
 * */
static int import_keys_from_keyring(char *fingerprints[], int len,
		const char *optional_gpg_home, struct str_array *imported_key_fprs)
{
	struct gc_gpgme_ctx ctx, gc_ctx;
	struct gpg_key_list gpg_keys;
	struct str_array key_fprs;

	if (!len)
		return 0;

	gpgme_context_init(&ctx, 0);
	gpgme_context_set_homedir(&ctx, optional_gpg_home);
	gpgme_context_init(&gc_ctx, 1);

	str_array_init(&key_fprs);

	for (int i = 0; i < len; i++)
		str_array_push(&key_fprs, fingerprints[i], NULL);

	// list and filter unusable keys
	fetch_gpg_keys(&ctx, &gpg_keys);
	filter_gpg_keys_by_predicate(&gpg_keys, gpg_keylist_filter_predicate,
			(void *) &key_fprs);

	// import keys into git-chat keyring
	struct gpg_key_list_node *key = gpg_keys.head;
	while (key) {
		gpgme_data_t key_data;

		gpgme_error_t ret = gpgme_data_new(&key_data);
		if (ret)
			GPG_FATAL("failed to allocate new gpg data buffer", ret);

		ret = gpgme_op_export_keys(ctx.gpgme_ctx,
				(gpgme_key_t[]) { key->key, NULL }, 0, key_data);
		if (ret)
			GPG_FATAL("unable to export key from external keyring", ret);

		ret = gpgme_data_seek(key_data, 0, SEEK_SET);
		if (ret)
			GPG_FATAL("failed to seek to beginning of gpg data buffer", ret);

		ret = gpgme_op_import(gc_ctx.gpgme_ctx, key_data);
		if (ret)
			GPG_FATAL("failed to import public key into git chat keyring", ret);

		gpgme_data_release(key_data);

		str_array_push(imported_key_fprs, key->key->fpr, NULL);

		key = key->next;
	}

	str_array_release(&key_fprs);

	release_gpg_key_list(&gpg_keys);
	gpgme_context_release(&ctx);
	gpgme_context_release(&gc_ctx);

	return 0;
}

static int import_keys(struct str_array *key_paths, char *fingerprints[],
		int len, const char *gpg_home)
{
	struct gc_gpgme_ctx gc_ctx;
	struct str_array imported_keys;
	struct strbuf gc_keys_dir, key_path;
	int ret = 0;

	if (!is_inside_git_chat_space())
		DIE("Where are you? It doesn't look like you're in the right directory.");

	strbuf_init(&key_path);
	strbuf_init(&gc_keys_dir);
	str_array_init(&imported_keys);

	// import keys from files
	import_keys_from_files(key_paths, &imported_keys);

	// import keys from keyring
	import_keys_from_keyring(fingerprints, len, gpg_home, &imported_keys);

	if (!imported_keys.len) {
		fprintf(stderr, "no keys were imported\n");
		ret = 1;
		goto fail;
	}

	if (get_keys_dir(&gc_keys_dir))
		FATAL("failed to obtain git-chat keys dir");

	// create the keys directory, if it does not exist
	safe_create_dir(gc_keys_dir.buff, NULL, S_IRWXU | S_IRGRP | S_IROTH);

	gpgme_context_init(&gc_ctx, 1);

	int git_index_file_count = 0;
	for (size_t f_index = 0; f_index < imported_keys.len; f_index++) {
		char *fingerprint = str_array_get(&imported_keys, f_index);
		strbuf_attach_fmt(&key_path, "%s/%s", gc_keys_dir.buff, fingerprint);

		ret = export_gpg_key(&gc_ctx, fingerprint, key_path.buff);
		if (ret > 0)
			FATAL("failed to export key with fingerprint %s", fingerprint);
		if (ret < 0) {
			WARN("unable to create the key file '%s'; a key with fingerprint "
				 "'%s' may have already been imported.", key_path.buff,
					fingerprint);

			strbuf_clear(&key_path);
			continue;
		}

		if (git_add_file_to_index(key_path.buff))
			FATAL("unable to update the git index with the exported gpg key");

		strbuf_clear(&key_path);
		git_index_file_count++;
	}

	gpgme_context_release(&gc_ctx);

	if (!git_index_file_count) {
		WARN("no keys left to import");
		ret = 1;
		goto fail;
	}

	commit_keys_from_index(&imported_keys);

fail:
	strbuf_release(&gc_keys_dir);
	strbuf_release(&key_path);
	str_array_release(&imported_keys);

	return ret;
}

int cmd_import_key(int argc, char *argv[])
{
	char *gpg_home_dir = NULL;
	struct str_array key_paths;
	int show_help = 0;

	const struct command_option options[] = {
			OPT_LONG_STRING("gpg-home", "path",
					"path to the gpg home directory", &gpg_home_dir),
			OPT_STRING_LIST('f', "file", "path",
					"path to exported public key file", &key_paths),
			OPT_BOOL('h', "help", "show usage and exit", &show_help),
			OPT_END()
	};

	str_array_init(&key_paths);

	argc = parse_options(argc, argv, options, 1, 1);
	if (show_help) {
		show_usage_with_options(import_key_cmd_usage, options, 0, NULL);
		return 0;
	}

	if (!key_paths.len && !argc) {
		show_usage_with_options(import_key_cmd_usage, options, 1,
				"error: nothing to do");
		str_array_release(&key_paths);
		return 1;
	}

	int ret = import_keys(&key_paths, argv, argc, gpg_home_dir);
	str_array_release(&key_paths);
	return ret;
}
