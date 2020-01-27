#include <string.h>
#include <fcntl.h>

#include "parse-options.h"
#include "gpg-common.h"
#include "git.h"
#include "working-tree.h"
#include "utils.h"
#include "fs-utils.h"

static const struct usage_string import_key_cmd_usage[] = {
		USAGE("git chat import-key [--gpg-home <path>] [--] <key fpr>..."),
		USAGE("git chat import-key (-f | --file) <path>"),
		USAGE("git chat import-key (-h | --help)"),
		USAGE_END()
};

static int import_keys_from_keyring(int, char *[], const char *);
static int import_key_from_files(struct str_array *);

int cmd_import_key(int argc, char *argv[])
{
	char *gpg_home_dir = NULL;
	struct str_array key_paths;
	int show_help = 0;

	const struct command_option options[] = {
			OPT_LONG_STRING("gpg-home", "path", "path to the gpg home directory", &gpg_home_dir),
			OPT_STRING_LIST('f', "file", "path", "path to exported public key file", &key_paths),
			OPT_BOOL('h', "help", "show usage and exit", &show_help),
			OPT_END()
	};

	str_array_init(&key_paths);

	argc = parse_options(argc, argv, options, 1, 1);
	if (show_help) {
		show_usage_with_options(import_key_cmd_usage, options, 0, NULL);
		return 0;
	}

	// fingerprint
	if (argc) {
		if (key_paths.len) {
			show_usage_with_options(import_key_cmd_usage, options, 1, "importing keys from an external keyring and from exported key files are mutually exclusive operations.");
			return 1;
		}

		return import_keys_from_keyring(argc, argv, gpg_home_dir);
	}

	if (key_paths.len) {
		if (gpg_home_dir) {
			show_usage_with_options(import_key_cmd_usage, options, 1, "importing keys from an external keyring and from exported key files are mutually exclusive operations.");
			return 1;
		}

		return import_key_from_files(&key_paths);
	}

	show_usage_with_options(import_key_cmd_usage, options, 1, "nothing to do");
	return 1;
}

/**
 * Filter any GPG keys in the given key list that do not have a fingerprint
 * specified in the given str_array data argument.
 * */
static int filter_gpg_keylist_by_fingerprints(struct _gpgme_key *key, void *data)
{
	struct str_array *fingerprints = (struct str_array *)data;
	for (size_t i = 0; i < fingerprints->len; i++) {
		struct str_array_entry *entry = str_array_get_entry(fingerprints, i);

		if (!strcmp(key->fpr, entry->string)) {
			entry->data = key;
			return 1;
		}
	}

	return 0;
}

static void publish_keys(void);

/**
 *
 * */
static int import_keys_from_keyring(int fpr_count, char *fpr[],
		const char *optional_gpg_home_dir)
{
	struct gc_gpgme_ctx ctx, gc_ctx;
	struct gpg_key_list gpg_keys;
	struct str_array fingerprints;
	struct strbuf keys_path, key_path;

	if (!is_inside_git_chat_space())
		DIE("Where are you? It doesn't look like you're in the right directory.");

	gpgme_context_init(&ctx, 0);
	gpgme_context_set_homedir(&ctx, optional_gpg_home_dir);

	strbuf_init(&key_path);
	str_array_init(&fingerprints);
	strbuf_init(&keys_path);

	if (get_keys_dir(&keys_path))
		FATAL("failed to obtain git-chat keys dir");

	// create the keys directory, if it does not exist
	safe_create_dir(keys_path.buff, NULL, S_IRWXU | S_IRGRP | S_IROTH);

	for (size_t i = 0; i < fpr_count; i++)
		str_array_push(&fingerprints, fpr[i], NULL);

	// filter for public keys by fingerprint
	int key_count = fetch_gpg_keys(&ctx, &gpg_keys);
	key_count -= filter_gpg_keys_by_predicate(&gpg_keys,
			&filter_gpg_secret_keys, NULL);
	key_count -= filter_gpg_keys_by_predicate(&gpg_keys,
			&filter_gpg_keylist_by_fingerprints, (void *)&fingerprints);

	// print warning for each unrecognized fingerprint
	for (size_t i = 0; i < fingerprints.len; i++) {
		struct str_array_entry *entry = str_array_get_entry(&fingerprints, i);
		if (!entry->data)
			WARN("could not find public gpg key with fingerprint '%s'", entry->string);
	}

	str_array_release(&fingerprints);

	if (!key_count)
		DIE("no public gpg keys could be found");

	gpgme_context_init(&gc_ctx, 1);

	// export public keys to working tree and import into git-chat keyring
	struct gpg_key_list_node *key = gpg_keys.head;
	while (key) {
		char *fingerprint = key->key->fpr;
		int status;

		strbuf_attach_fmt(&key_path, "%s/%s", keys_path.buff, fingerprint);
		status = export_gpg_key(&ctx, fingerprint, key_path.buff);
		if (status > 0)
			FATAL("failed to export key with fingerprint %s", fingerprint);
		else if (status < 0)
			DIE("unable to create the key file '%s'; a key with fingerprint "
					"'%s' may have already been imported.", key_path.buff, fingerprint);

		if (git_add_file_to_index(key_path.buff))
			DIE("unable to update the git index with the exported gpg key");

		if (!import_gpg_key(&gc_ctx, key_path.buff, NULL))
			DIE("failed to import keys into git-chat keyring");

		strbuf_clear(&key_path);
		key = key->next;
	}

	strbuf_release(&keys_path);
	strbuf_release(&key_path);
	str_array_release(&fingerprints);

	release_gpg_key_list(&gpg_keys);
	gpg_context_release(&ctx);
	gpg_context_release(&gc_ctx);

	publish_keys();

	return 0;
}

/**
 * Import a GPG key from the given file into the git-chat keyring, and
 * export the same key to a file to be tracked in the commit graph.
 * */
static int import_key_from_files(struct str_array *key_files)
{
	struct gc_gpgme_ctx gc_ctx;
	struct str_array imported_keys;
	struct strbuf keys_dir;
	struct strbuf key_path;

	if (!is_inside_git_chat_space())
		DIE("Where are you? It doesn't look like you're in the right directory.");

	gpgme_context_init(&gc_ctx, 1);
	str_array_init(&imported_keys);
	strbuf_init(&keys_dir);
	strbuf_init(&key_path);

	int keys_imported = 0;
	for (size_t f_index = 0; f_index < key_files->len; f_index++) {
		char *file_path = str_array_get(key_files, f_index);

		// import key from file into git-chat keyring
		int key_count = import_gpg_key(&gc_ctx, file_path, &imported_keys);
		if (!key_count)
			WARN("unable to import key from file '%s'", file_path);

		keys_imported += key_count;
	}

	if (!keys_imported)
		DIE("failed to import keys into git-chat keyring");

	if (get_keys_dir(&keys_dir))
		FATAL("failed to obtain git-chat keys dir");

	// export keys to working tree
	for (size_t i = 0; i < keys_imported; i++) {
		char *fpr = str_array_get(&imported_keys, i);
		int status;

		strbuf_attach_fmt(&key_path, "%s/%s", keys_dir.buff, fpr);

		status = export_gpg_key(&gc_ctx, fpr, key_path.buff);
		if (status > 0)
			FATAL("failed to export key with fingerprint %s", fpr);
		else if (status < 0)
			DIE("unable to create the key file '%s'; a key with fingerprint "
				"'%s' may have already been imported.", key_path.buff, fpr);

		if (git_add_file_to_index(key_path.buff))
			DIE("unable to update the git index with the exported gpg key");

		strbuf_clear(&key_path);
	}

	gpg_context_release(&gc_ctx);
	str_array_release(&imported_keys);
	strbuf_release(&keys_dir);
	strbuf_release(&key_path);

	publish_keys();

	return 0;
}

static void publish_keys(void)
{
	struct strbuf message;
	strbuf_init(&message);

	if (get_author_identity(&message))
		strbuf_attach_str(&message, "unknown user");

	strbuf_attach_str(&message, " joined the channel");

	if (git_commit_index_with_options(message.buff, "--no-gpg-sign", "--no-verify", NULL))
		DIE("unable to commit exported gpg key(s) to tree");

	strbuf_release(&message);
}
