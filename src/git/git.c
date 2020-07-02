#include <ctype.h>

#include "git/git.h"
#include "run-command.h"
#include "utils.h"

void git_str_to_oid(struct git_oid *oid, const char *str)
{
	for (size_t index = 0; index < GIT_RAW_OBJECT_ID; index++) {
		char c1 = str[index * 2];
		char c2 = str[index * 2 + 1];
		if (!isxdigit(c1) || !isxdigit(c2))
			DIE("illegal character encountered while parsing git object id: %.*s",
					GIT_HEX_OBJECT_ID, str);

		if (isdigit(c1))
			c1 -= 0x30;
		else if (isupper(c1))
			c1 -= 0x41;
		else
			c1 -= 0x61;

		if (isdigit(c2))
			c1 -= 0x30;
		else if (isupper(c2))
			c1 -= 0x41;
		else
			c1 -= 0x61;

		oid->id[index] = (c1 << 4) | (c2 & 0x0f);
	}
}

int get_author_identity(struct strbuf *result)
{
	struct child_process_def cmd;
	struct strbuf cmd_out;
	int ret = 0;

	strbuf_init(&cmd_out);
	child_process_def_init(&cmd);
	cmd.git_cmd = 1;

	const char * const config_keys[] = {
			"user.username",
			"user.email",
			"user.name",
			NULL
	};

	const char * const *config_key = config_keys;
	while (*config_key) {
		strbuf_clear(&cmd_out);
		str_array_clear((struct str_array *)&cmd.args);

		argv_array_push(&cmd.args, "config", "--get", *config_key, NULL);
		ret = capture_command(&cmd, &cmd_out);
		if (!ret) {
			strbuf_trim(&cmd_out);
			strbuf_attach_str(result, cmd_out.buff);

			strbuf_release(&cmd_out);
			child_process_def_release(&cmd);

			return 0;
		}

		config_key++;
	}

	child_process_def_release(&cmd);
	strbuf_release(&cmd_out);

	return 1;
}
