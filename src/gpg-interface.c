#include <string.h>
#include <errno.h>
#include <sys/stat.h>
#include <dirent.h>
#include <locale.h>

#include "gpg-interface.h"
#include "utils.h"

static struct gpg_key_list_node *gpg_key_list_push(struct gpg_key_list *list,
		struct _gpgme_key *key);
NORETURN static void GPG_FATAL(const char *msg, gpgme_error_t err);

const char *get_gpgme_library_version()
{
	return gpgme_check_version(NULL);
}

void init_gpgme_openpgp_engine(void)
{
	gpgme_error_t err;

	LOG_DEBUG("Initializing GPGME with GPGME_PROTOCOL_OpenPGP engine");

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
	if (err && gpgme_err_code(err) != GPG_ERR_NO_ERROR)
		GPG_FATAL("failed to load OpenPGP engine using installed version of GPGME", err);

	struct _gpgme_engine_info *enginfo;
	err = gpgme_get_engine_info(&enginfo);
	if (err)
		GPG_FATAL("failed to load gpgme engine info", err);

	const char *engine_version = enginfo->version;
	const char *protocol_name = (char *) gpgme_get_protocol_name(GPGME_PROTOCOL_OpenPGP);
	const char *engine_executable = enginfo->file_name;
	const char *engine_config_dir = enginfo->home_dir;
	const char *default_config_dif = gpgme_get_dirinfo("homedir");

	if (!engine_version)
		engine_version = "cannot be determined";
	if (!protocol_name)
		protocol_name = "invalid protocol number";
	if (!engine_executable)
		engine_executable = "unknown";
	if (!engine_config_dir)
		engine_config_dir = default_config_dif;
	if (!engine_config_dir)
		engine_config_dir = "unknown";

	LOG_INFO("crypto engine version number: %s, engine protocol: %s, "
		  "engine executable: %s, engine config directory: %s", engine_version,
			 protocol_name, engine_executable, engine_config_dir);
}

int rebuild_gpg_keyring(const char *gpg_homedir, const char *keys_dir)
{
	struct gpgme_context *ctx;
	gpgme_error_t err;

	err = gpgme_new(&ctx);
	if (err)
		GPG_FATAL("failed to create GPGME context", err);

	if (mkdir(gpg_homedir, 0700) < 0 && errno != EEXIST)
		FATAL("unable to create gpg home directory '%s'", gpg_homedir);

	err = gpgme_ctx_set_engine_info(ctx, GPGME_PROTOCOL_OpenPGP, NULL, gpg_homedir);
	if (err)
		GPG_FATAL("failed to mutate GPGME engine homedir", err);

	DIR *dir;
	dir = opendir(keys_dir);
	if (!dir)
		FATAL("unable to open directory '%s'", keys_dir);

	// construct a list of paths to all keys in the keys dir
	struct str_array key_files;
	str_array_init(&key_files);

	struct dirent *ent;
	while ((ent = readdir(dir)) != NULL) {
		struct stat st_key;
		if (!strcmp(".", ent->d_name) || !strcmp("..", ent->d_name))
			continue;

		struct strbuf file_path;
		strbuf_init(&file_path);
		strbuf_attach_fmt(&file_path, "%s/%s", keys_dir, ent->d_name);

		if (lstat(file_path.buff, &st_key) && errno == ENOENT)
			FATAL("unable to stat '%s'", file_path.buff);

		if (!S_ISREG(st_key.st_mode))
			DIE("cannot import key '%s'; file is not a valid gpg public key", file_path.buff);

		char *key_path = strbuf_detach(&file_path);
		struct str_array_entry *entry = str_array_insert_nodup(&key_files,
				key_path, key_files.len);

		// get key data
		struct gpgme_data *key;
		err = gpgme_data_new_from_file(&key, key_path, 1);
		if (err) {
			LOG_ERROR("Failed to read key file '%s'", key_path);
			GPG_FATAL("GPGME failed to read key file", err);
		}

		entry->data = key;
	}

	closedir(dir);

	LOG_INFO("Attempting to import %d gpg keys from %s", key_files.len, keys_dir);

	int keys_imported = 0;
	for (size_t index = 0; index < key_files.len; index++) {
		struct str_array_entry *entry = str_array_get_entry(&key_files, index);
		err = gpgme_op_import(ctx, entry->data);
		if (err) {
			LOG_ERROR("Failed to import key '%s'", entry->string);
			GPG_FATAL("GPGME failed to import key", err);
		}

		gpgme_data_release(entry->data);
	}

	str_array_release(&key_files);
	gpgme_release(ctx);

	return keys_imported;
}

void encrypt_plaintext_message(const char *gpg_homedir, const struct strbuf *message,
		struct strbuf *output, struct gpg_key_list *recipients)
{
	struct gpgme_context *ctx;
	gpgme_error_t err;

	err = gpgme_new(&ctx);
	if (err)
		GPG_FATAL("failed to create GPGME context", err);

	if (mkdir(gpg_homedir, 0700) < 0 && errno != EEXIST)
		FATAL("unable to create gpg home directory '%s'", gpg_homedir);

	err = gpgme_ctx_set_engine_info(ctx, GPGME_PROTOCOL_OpenPGP, NULL, gpg_homedir);
	if (err)
		GPG_FATAL("failed to mutate GPGME engine homedir", err);

	gpgme_set_armor(ctx, 1);

	struct str_array keys;
	str_array_init(&keys);

	struct gpg_key_list_node *node = recipients->head;
	while (node) {
		struct str_array_entry *entry = str_array_insert_nodup(&keys, NULL, keys.len);
		entry->data = node->key;

		node = node->next;
	}

	size_t keys_len = 0;
	struct _gpgme_key **keys_array = (struct _gpgme_key **)str_array_detach_data(&keys, &keys_len);
	if (!keys_len)
		BUG("no gpg keys given to encrypt_plaintext_message()");

	struct gpgme_data *message_in;
	struct gpgme_data *message_out;
	err = gpgme_data_new_from_mem(&message_in, message->buff, message->len, 0);
	if (err)
		GPG_FATAL("unable to create GPGME memory data buffer from plaintext message", err);

	err = gpgme_data_new(&message_out);
	if (err)
		GPG_FATAL("unable to create GPGME data buffer for encrypted ciphertext", err);

	err = gpgme_op_encrypt(ctx, keys_array, GPGME_ENCRYPT_ALWAYS_TRUST | GPGME_ENCRYPT_NO_ENCRYPT_TO,
			message_in,  message_out);
	if (err) {
		if (gpgme_err_code(err) == GPG_ERR_INV_VALUE)
			BUG("invalid pointer passed to gpgme_op_encrypt(...)");

		if (gpgme_err_code(err) == GPG_ERR_UNUSABLE_PUBKEY) {
			struct _gpgme_op_encrypt_result *result = gpgme_op_encrypt_result (ctx);

			fprintf(stderr, "cannot encrypt message with invalid recipient:\n");

			struct _gpgme_invalid_key *invalid_key = result->invalid_recipients;
			while (invalid_key) {
				fprintf(stderr, "\t%s - Reason: %s\n", invalid_key->fpr, gpgme_strerror(invalid_key->reason));

				invalid_key = invalid_key->next;
			}
		}

		GPG_FATAL("GPGME unable to encrypt message", err);
	}

	int ret = gpgme_data_seek (message_out, 0, SEEK_SET);
	if (ret)
		GPG_FATAL("failed to seek to beginning of gpgme data buffer", err);

	char temporary_buffer[1024];
	while ((ret = gpgme_data_read(message_out, temporary_buffer, 1024)) > 0)
		strbuf_attach(output, temporary_buffer, 1024);

	if (ret < 0)
		GPG_FATAL("failed to read from gpgme data buffer", err);

	// free the array but leave the gpg keys intact; they are owned by the caller
	free(keys_array);

	gpgme_data_release (message_in);
	gpgme_data_release (message_out);
	gpgme_release(ctx);
}

int get_gpg_keys_from_keyring(const char *gpg_homedir, struct gpg_key_list *keys)
{
	struct gpgme_context *ctx;
	gpgme_error_t err;

	err = gpgme_new(&ctx);
	if (err)
		GPG_FATAL("failed to create GPGME context", err);

	if (mkdir(gpg_homedir, 0700) < 0 && errno != EEXIST)
		FATAL("unable to create gpg home directory '%s'", gpg_homedir);

	err = gpgme_ctx_set_engine_info(ctx, GPGME_PROTOCOL_OpenPGP, NULL, gpg_homedir);
	if (err)
		GPG_FATAL("failed to mutate GPGME engine homedir", err);

	struct _gpgme_key *key;
	err = gpgme_op_keylist_start(ctx, NULL, 0);
	if (err)
		GPG_FATAL("failed to begin a gpg key listing operation", err);

	keys->head = NULL;
	keys->tail = NULL;

	int keys_fetched = 0;
	while (!(err = gpgme_op_keylist_next(ctx, &key))) {
		gpg_key_list_push(keys, key);
		keys_fetched++;
	}

	if (gpg_err_code(err) != GPG_ERR_EOF)
		GPG_FATAL("failed to retrieve gpg keys from keyring", err);

	gpgme_release(ctx);

	return keys_fetched;
}

int filter_gpg_keys_by_predicate(struct gpg_key_list *keys,
		int (*predicate)(struct _gpgme_key *, void *), void *optional_data)
{
	int filtered_keys = 0;
	struct gpg_key_list_node *node = keys->head;
	while (node) {
		struct gpg_key_list_node *next = node->next;
		struct gpg_key_list_node *prev = node->prev;
		if (predicate(node->key, optional_data)) {
			node = next;
			continue;
		}

		if (keys->head == node)
			keys->head = next;
		if (keys->tail == node)
			keys->tail = prev;

		if (prev)
			prev->next = next;
		if (next)
			next->prev = prev;

		gpgme_key_release(node->key);
		free(node);

		node = next;
		filtered_keys++;
	}

	return filtered_keys;
}

int filter_gpg_unusable_keys(struct _gpgme_key *key, void *data)
{
	// Unused
	(void) data;

	if (key->expired)
		return 0;
	if (key->disabled)
		return 0;
	if (key->invalid)
		return 0;

	if (!key->can_encrypt)
		return 0;

	return 1;
}

int filter_gpg_secret_keys(struct _gpgme_key *key, void *data)
{
	// Unused
	(void) data;

	return !key->secret;
}

void release_gpg_key_list(struct gpg_key_list *keys)
{
	struct gpg_key_list_node *node = keys->head;
	struct gpg_key_list_node *next;
	while (node) {
		next = node->next;

		gpgme_key_release(node->key);
		free(node);

		node = next;
	}

	keys->head = NULL;
	keys->tail = NULL;
}

/**
 * Push a gpgme key structure to a list of gpg keys, returning the newly allocated
 * gpg_key_list node structure.
 * */
static struct gpg_key_list_node *gpg_key_list_push(struct gpg_key_list *list,
		struct _gpgme_key *key)
{
	struct gpg_key_list_node *node = (struct gpg_key_list_node *) malloc(sizeof(struct gpg_key_list_node));
	if (!node)
		FATAL(MEM_ALLOC_FAILED);

	node->key = key;
	node->prev = list->tail;
	node->next = NULL;

	if (!list->head)
		list->head = node;
	else
		list->tail->next = node;

	list->tail = node;

	return node;
}

/**
 * Wrapper for FATAL(...) util. Includes gpgme error information.
 * */
NORETURN static void GPG_FATAL(const char *msg, gpgme_error_t err)
{
	FATAL("%s: %s: %s", msg, gpgme_strsource(err), gpgme_strerror(err));
}
