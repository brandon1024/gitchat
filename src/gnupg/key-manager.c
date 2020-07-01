#include <stdlib.h>
#include <string.h>
#include <fcntl.h>
#include <errno.h>
#include <sys/stat.h>
#include <dirent.h>
#include <unistd.h>

#include "gnupg/key-manager.h"
#include "working-tree.h"
#include "utils.h"

int import_gpg_key(struct gc_gpgme_ctx *ctx, const char *key_file_path,
		struct str_array *imported_key_fingerprints)
{
	gpgme_error_t err;
	struct gpgme_data *key_data;
	struct strbuf keys_dir;
	int keys_imported = 0;

	err = gpgme_data_new_from_file(&key_data, key_file_path, 1);
	if (err)
		FATAL("Failed to read key file '%s'", key_file_path);

	err = gpgme_op_import(ctx->gpgme_ctx, key_data);
	switch (err) {
		case GPG_ERR_INV_VALUE:
			BUG("failed to import key from file");
		case GPG_ERR_NO_DATA:
			BUG("failed to import key from file; file or buffer was empty");
		case GPG_ERR_NO_ERROR:
		default:
			LOG_DEBUG("Successfully imported key from file");
	}

	gpgme_data_release(key_data);

	strbuf_init(&keys_dir);
	if (get_keys_dir(&keys_dir))
		FATAL("failed to obtain git-chat keys dir");

	gpgme_import_result_t import_result = gpgme_op_import_result(ctx->gpgme_ctx);
	gpgme_import_status_t result = import_result->imports;
	while (result) {
		if (result->result != GPG_ERR_NO_ERROR)
			DIE("failed to import GPG key");

		LOG_DEBUG("imported key with fingerprint %s", result->fpr);
		if (imported_key_fingerprints)
			str_array_push(imported_key_fingerprints, result->fpr, NULL);

		result = result->next;
		keys_imported++;
	}

	return keys_imported;
}

int export_gpg_key(struct gc_gpgme_ctx *ctx, const char *fingerprint,
		const char *file_path)
{
	gpgme_error_t err;
	gpgme_data_t key_data;

	LOG_DEBUG("exporting key with fingerprint %s to file '%s'", fingerprint, file_path);

	err = gpgme_data_new(&key_data);
	if (err) {
		LOG_ERROR("%s: %s", gpgme_strsource(err), gpgme_strerror(err));
		return 1;
	}

	err = gpgme_op_export(ctx->gpgme_ctx, fingerprint, 0, key_data);
	if (err) {
		LOG_ERROR("%s: %s", gpgme_strsource(err), gpgme_strerror(err));
		gpgme_data_release(key_data);
		return 1;
	}

	int fd = open(file_path, O_WRONLY | O_CREAT | O_EXCL,
			S_IRUSR | S_IWUSR | S_IRGRP | S_IROTH);
	if (fd < 0) {
		LOG_ERROR(FILE_OPEN_FAILED, file_path);
		return -1;
	}

	err = gpgme_data_seek(key_data, 0, SEEK_SET);
	if (err) {
		LOG_ERROR("%s: %s", gpgme_strsource(err), gpgme_strerror(err));
		gpgme_data_release(key_data);
		close(fd);

		return 1;
	}

	ssize_t bytes_read;
	unsigned char buffer[1024];
	while ((bytes_read = gpgme_data_read(key_data, buffer, 1024)) > 0) {
		if (recoverable_write(fd, buffer, bytes_read) != bytes_read)
			FATAL("file write failed");
	}

	if (bytes_read < 0)
		FATAL("unable to read data from GPGME data buffer");

	close(fd);
	gpgme_data_release(key_data);

	return 0;
}

static struct gpg_key_list_node *gpg_key_list_push(struct gpg_key_list *,
		struct _gpgme_key *);

int rebuild_gpg_keyring(struct gc_gpgme_ctx *ctx, const char *keys_dir)
{
	gpgme_error_t err;
	int errsv = errno;

	LOG_INFO("Rebuilding gpg keyring");

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

		LOG_TRACE("Attempting to read gpg key file '%s'", file_path.buff);

		if (lstat(file_path.buff, &st_key) && errno == ENOENT)
			FATAL("unable to stat '%s'", file_path.buff);

		if (!S_ISREG(st_key.st_mode))
			DIE("cannot import key '%s'; file is not a valid gpg public key", file_path.buff);

		char *key_path = strbuf_detach(&file_path);
		struct str_array_entry *entry = str_array_insert_nodup(&key_files,
				key_path, key_files.len);

		struct gpgme_data *key;
		err = gpgme_data_new_from_file(&key, key_path, 1);
		if (err) {
			LOG_ERROR("Failed to read key file '%s'", key_path);
			GPG_FATAL("GPGME failed to read key file", err);
		}

		LOG_TRACE("Successfully read file '%s' into gpg data buffer", key_path);

		entry->data = key;
	}

	closedir(dir);

	LOG_INFO("Attempting to import %d gpg keys from %s", key_files.len, keys_dir);

	int keys_imported = 0;
	for (size_t index = 0; index < key_files.len; index++) {
		struct str_array_entry *entry = str_array_get_entry(&key_files, index);
		err = gpgme_op_import(ctx->gpgme_ctx, entry->data);
		if (err) {
			LOG_ERROR("Failed to import key '%s'", entry->string);
			GPG_FATAL("GPGME failed to import key", err);
		}

		LOG_TRACE("Successfully imported gpg key '%s'", entry->string);

		gpgme_data_release(entry->data);
	}

	str_array_release(&key_files);

	LOG_INFO("Successfully imported %d gpg keys from %s", key_files.len, keys_dir);

	errno = errsv;
	return keys_imported;
}

int fetch_gpg_keys(struct gc_gpgme_ctx *ctx, struct gpg_key_list *keys)
{
	gpgme_error_t err;
	int errsv = errno;

	LOG_INFO("Fetching gpg keys from keyring under home directory");

	struct _gpgme_key *key;
	err = gpgme_op_keylist_start(ctx->gpgme_ctx, NULL, 0);
	if (err)
		GPG_FATAL("failed to begin a gpg key listing operation", err);

	keys->head = NULL;
	keys->tail = NULL;

	int keys_fetched = 0;
	while (!(err = gpgme_op_keylist_next(ctx->gpgme_ctx, &key))) {
		gpg_key_list_push(keys, key);
		keys_fetched++;

		LOG_TRACE("Fetched gpg key with fingerprint: %s", key->fpr);
	}

	if (gpg_err_code(err) != GPG_ERR_EOF)
		GPG_FATAL("failed to retrieve gpg keys from keyring", err);

	LOG_INFO("Successfully fetched %d gpg keys", keys_fetched);

	errno = errsv;
	return keys_fetched;
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
