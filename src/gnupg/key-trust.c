#include <fcntl.h>
#include <unistd.h>
#include <string.h>

#include "gnupg/key-trust.h"
#include "strbuf.h"
#include "fs-utils.h"
#include "utils.h"

ssize_t read_trust_list(struct str_array *trusted_keys)
{
	struct strbuf trusted_keys_file_path;
	strbuf_init(&trusted_keys_file_path);

	if (get_cwd(&trusted_keys_file_path))
		FATAL("unable to obtain the current working directory from getcwd()");

	strbuf_attach_str(&trusted_keys_file_path, "/.git/.trusted-keys");

	int fd = open(trusted_keys_file_path.buff, O_RDONLY);
	strbuf_release(&trusted_keys_file_path);

	if (fd < 0)
		return -1;

	struct strbuf file_contents;
	struct str_array fingerprints;

	strbuf_init(&file_contents);
	str_array_init(&fingerprints);

	strbuf_attach_fd(&file_contents, fd);
	close(fd);

	size_t line_count = strbuf_split(&file_contents, "\n", &fingerprints);
	strbuf_release(&file_contents);

	for (size_t i = 0; i < fingerprints.len; i++) {
		const char *fpr = str_array_get(&fingerprints, i);
		if (strlen(fpr))
			str_array_push(trusted_keys, fpr, NULL);
	}

	str_array_release(&fingerprints);

	return (ssize_t) line_count;
}
