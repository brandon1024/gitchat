#include <string.h>
#include <errno.h>
#include <sys/stat.h>

#include "working-tree.h"
#include "run-command.h"
#include "fs-utils.h"
#include "utils.h"

#define GIT_CHAT_DIR	".git-chat"
#define KEYS_DIR		".git-chat/keys"
#define GNUPG_HOME_DIR	".git/.gnupg"
#define CHAT_CACHE_DIR	".git/chat-cache"

int is_inside_git_chat_space()
{
	int errsv = errno;

	struct stat sb;
	if (stat(GIT_CHAT_DIR, &sb) == -1 || !S_ISDIR(sb.st_mode)) {
		LOG_DEBUG("Cannot stat .git-chat directory; %s", strerror(errno));
		errno = errsv;
		return 0;
	}

	if (stat(".git", &sb) == -1 || !S_ISDIR(sb.st_mode)) {
		LOG_DEBUG("Cannot stat .git directory; %s", strerror(errno));
		errno = errsv;
		return 0;
	}

	struct child_process_def cmd;
	child_process_def_init(&cmd);
	cmd.git_cmd = 1;
	cmd.std_fd_info = STDIN_NULL | STDOUT_NULL | STDERR_NULL;
	argv_array_push(&cmd.args, "rev-parse", "--is-inside-work-tree", NULL);

	//if 'git rev-parse --is-inside-work-tree' return with a zero exit status,
	//its safe enough to assume we are in a git-chat space.
	int status = run_command(&cmd);
	child_process_def_release(&cmd);

	errno = errsv;
	return status == 0;
}

static int get_dir(const char *dir, struct strbuf *buffer);

int get_gpg_homedir(struct strbuf *path)
{
	return get_dir(GNUPG_HOME_DIR, path);
}

int get_keys_dir(struct strbuf *path)
{
	return get_dir(KEYS_DIR, path);
}

int get_git_chat_dir(struct strbuf *path)
{
	return get_dir(GIT_CHAT_DIR, path);
}

int get_chat_cache_dir(struct strbuf *path)
{
	return get_dir(CHAT_CACHE_DIR, path);
}

static int get_dir(const char *dir, struct strbuf *buffer)
{
	if (buffer->len)
		BUG("buffer must be empty");

	struct strbuf cwd_buff;
	strbuf_init(&cwd_buff);
	if (get_cwd(&cwd_buff)) {
		strbuf_release(&cwd_buff);
		return 1;
	}

	strbuf_attach_fmt(buffer, "%s/%s", cwd_buff.buff, dir);
	strbuf_release(&cwd_buff);
	return 0;
}
