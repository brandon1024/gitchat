#include <stdio.h>
#include <string.h>
#include <errno.h>
#include <sys/stat.h>
#include <dirent.h>
#include <fcntl.h>

#include "gpg-interface.h"
#include "run-command.h"
#include "fs-utils.h"
#include "utils.h"

static char *gpg_executable;

static int import_gpg_keys_to_keyring(const char *keyring_path,
		struct str_array *key_files);

int find_gpg_executable(struct strbuf *path)
{
	struct child_process_def cmd;
	child_process_def_init(&cmd);
	child_process_def_stderr(&cmd, STDERR_NULL);
	cmd.git_cmd = 1;
	argv_array_push(&cmd.args, "config", "--get", "gpg.program", NULL);

	struct strbuf output_capture;
	strbuf_init(&output_capture);
	int status = capture_command(&cmd, &output_capture);

	char *executable_path = NULL;
	if (!status) {
		strbuf_trim(&output_capture);

		if (is_executable(output_capture.buff))
			executable_path = strbuf_detach(&output_capture);
		else
			executable_path = find_in_path(output_capture.buff);
	}

	strbuf_release(&output_capture);
	child_process_def_release(&cmd);

	if (!executable_path)
		executable_path = find_in_path("gpg");
	if (!executable_path)
		executable_path = find_in_path("gpg2");

	if (executable_path)
		strbuf_attach_str(path, executable_path);

	free(executable_path);
	return executable_path == NULL;
}

int rebuild_gpg_keyring(const char *keyring_path, const char *keys_path)
{
	DIR *dir;
	struct dirent *ent;
	struct stat sb;
	int status = 0;

	struct strbuf gpg_executable_path;
	strbuf_init(&gpg_executable_path);
	if (find_gpg_executable(&gpg_executable_path))
		DIE("unable to locate gpg executable");

	gpg_executable = strbuf_detach(&gpg_executable_path);

	// delete keyring, if exists
	if (stat(keyring_path, &sb) == 0) {
		status = remove(keyring_path);
		if (status < 0) {
			LOG_ERROR("Failed to remove keyring '%s'", keyring_path);
			free(gpg_executable);
			return 1;
		}
	}

	dir = opendir(keys_path);
	if (!dir) {
		LOG_ERROR("unable to open directory '%s'", keys_path);
		free(gpg_executable);
		return 1;
	}

	struct str_array key_files;
	str_array_init(&key_files);

	while ((ent = readdir(dir)) != NULL) {
		struct stat st_from;
		if (!strcmp(".", ent->d_name) || !strcmp("..", ent->d_name))
			continue;

		struct strbuf file_path;
		strbuf_init(&file_path);
		strbuf_attach_fmt(&file_path, "%s/%s", keys_path, ent->d_name);

		if (lstat(file_path.buff, &st_from) && errno == ENOENT)
			FATAL("unable to stat directory '%s'", file_path.buff);

		if (!S_ISREG(st_from.st_mode))
			DIE("cannot import key '%s'; file is not a valid gpg public key", file_path.buff);

		str_array_insert_nodup(&key_files, strbuf_detach(&file_path), key_files.len);
		strbuf_release(&file_path);
	}

	closedir(dir);

	LOG_INFO("Importing %d gpg keys from %s", key_files.len, keys_path);

	// import keys in chunks of 8
	struct str_array tmp;
	str_array_init(&tmp);
	while(key_files.len > 0) {
		char *file_path = str_array_remove(&key_files, key_files.len - 1);
		str_array_insert_nodup(&tmp, file_path, tmp.len);

		// import keys in chunks of 8, and if key_files is empty
		if (tmp.len % 8 != 0 && key_files.len)
			continue;

		status = import_gpg_keys_to_keyring(keyring_path, &tmp);
		if (status) {
			LOG_ERROR("Failed to import keys into keyring; gpg exited with status %d", status);
			break;
		}

		str_array_clear(&tmp);
	}

	str_array_release(&key_files);
	str_array_release(&tmp);
	free(gpg_executable);

	return status;
}

int retrieve_fingerprints_from_keyring(const char *keyring_path,
		struct str_array *fingerprints)
{
	int fingerprints_added = 0;

	struct strbuf path_buff;
	strbuf_init(&path_buff);
	int ret = find_gpg_executable(&path_buff);
	if (ret)
		DIE("unable to locate gpg executable");

	struct child_process_def cmd;
	child_process_def_init(&cmd);
	cmd.executable = path_buff.buff;
	child_process_def_stdin(&cmd, STDIN_NULL);
	argv_array_push(&cmd.args, "--no-default-keyring", "--keyring", keyring_path,
			"-k", "--with-colons", NULL);

	struct strbuf output_buff;
	strbuf_init(&output_buff);
	ret = capture_command(&cmd, &output_buff);
	if (ret)
		DIE("failed to retrieve fingerprints from keyring; gpg exited with %d", ret);

	child_process_def_release(&cmd);
	strbuf_release(&path_buff);

	//parse output
	strbuf_trim(&output_buff);
	struct str_array lines;
	str_array_init(&lines);
	strbuf_split(&output_buff, "\n", &lines);

	for (size_t i = 0; i < lines.len; i++) {
		struct strbuf line_buf;
		strbuf_init(&line_buf);
		struct str_array fields;
		str_array_init(&fields);

		strbuf_attach_str(&line_buf, lines.strings[i]);
		strbuf_split(&line_buf, ":", &fields);

		const char *field_0 = str_array_get(&fields, 0);
		const char *field_9 = str_array_get(&fields, 9);
		if (field_0 && !strcmp("fpr", field_0)) {
			LOG_TRACE("Fingerprint line from gpg output: %s", line_buf.buff);

			if (field_9 && strlen(field_9)) {
				str_array_push(fingerprints, field_9, NULL);
				fingerprints_added++;
			} else {
				LOG_WARN(
						"Unable to parse gpg output:\n"
						"GPG colon-formatted output is missing the expected fingerprint value\n"
						"in field 10. Skipping this fingerprint entry.\n"
				);
			}
		}

		strbuf_release(&line_buf);
		str_array_release(&fields);
	}

	strbuf_release(&output_buff);
	str_array_release(&lines);

	if (fingerprints_added)
		LOG_INFO("Successfully read %d key fingerprints from gpg output", fingerprints_added);
	else
		LOG_INFO("No key fingerprints were found in keyring %s.", keyring_path);

	return fingerprints_added;
}

/**
 * Import into the given keyring one or more gpg keys.
 *
 * The keyring_path argument need not point to an existing keyring, since gpg
 * will create the keyring if necessary.
 *
 * The key_files argument must contain at least one entry. Each entry must point
 * to a valid gpg key.
 *
 * Returns the exit status of the gpg program used to import the keys.
 * */
static int import_gpg_keys_to_keyring(const char *keyring_path,
		struct str_array *key_files)
{
	struct child_process_def cmd;
	child_process_def_init(&cmd);
	cmd.std_fd_info = STDIN_NULL | STDOUT_NULL | STDERR_NULL;
	cmd.executable = gpg_executable;
	argv_array_push(&cmd.args, "--no-default-keyring", "--keyring", keyring_path,
			"--import", NULL);

	for (size_t i = 0; i < key_files->len; i++)
		argv_array_push(&cmd.args, key_files->strings[i], NULL);

	int status = run_command(&cmd);
	child_process_def_release(&cmd);

	return status;
}
