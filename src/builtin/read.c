#include <unistd.h>

#include "git/graph-traversal.h"
#include "gnupg/gpg-common.h"
#include "gnupg/decryption.h"
#include "working-tree.h"
#include "parse-options.h"
#include "paging.h"
#include "utils.h"

static const struct usage_string read_cmd_usage[] = {
		USAGE("git chat read [(-n | --max-count) <n>] [--no-color] [<commit hash>]"),
		USAGE("git chat read (-h | --help)"),
		USAGE_END()
};

struct graph_traversal_context {
	int no_color;
	struct gc_gpgme_ctx *gpg_ctx;
};

/**
 * Commit traversal callback that attempts to decrypt the commit message body
 * and pretty-prints the message to standard output.
 *
 * Returns zero.
 * */
static int commit_traversal_cb(struct git_commit *commit, void *data)
{
	struct graph_traversal_context *ctx = (struct graph_traversal_context *) data;
	struct gc_gpgme_ctx *gpg_ctx = ctx->gpg_ctx;
	int no_color = ctx->no_color;

	struct strbuf decrypted_text;
	strbuf_init(&decrypted_text);

	int ret = decrypt_asymmetric_message(gpg_ctx, &commit->body, &decrypted_text);
	if (!ret) {
		// decryption successful
		pretty_print_message(commit, &decrypted_text, DECRYPTED, no_color, STDOUT_FILENO);
	} else if (ret > 0) {
		// commit body is not gpg message; print commit message body
		pretty_print_message(commit, &commit->body, PLAINTEXT, no_color, STDOUT_FILENO);
	} else {
		strbuf_clear(&decrypted_text);
		strbuf_attach_str(&decrypted_text, "message could not be decrypted.");
		pretty_print_message(commit, &decrypted_text, UNKNOWN_ERROR, no_color, STDOUT_FILENO);
	}

	fflush(stdout);

	strbuf_release(&decrypted_text);

	return 0;
}

/**
 * Read messages in the configured pager, starting at the given `commit`. If
 * limit is a positive integer, at most `limit` messages are shown. If `no_color`
 * is non-zero, ANSI color escape sequences are not written to output.
 *
 * Returns zero.
 * */
static int read_messages(const char *commit, int limit, int no_color)
{
	struct gc_gpgme_ctx gpg_ctx;
	gpgme_context_init(&gpg_ctx, 0);

	pager_start(GIT_CHAT_PAGER_RAW_CTRL_CHR | GIT_CHAT_PAGER_CLR_SCRN);

	struct graph_traversal_context ctx = { .no_color = no_color, .gpg_ctx = &gpg_ctx };
	int ret = traverse_commit_graph(commit, limit, commit_traversal_cb, &ctx);
	if (ret)
		FATAL("commit graph traversal failed");

	gpgme_context_release(&gpg_ctx);
	return 0;
}

int cmd_read(int argc, char *argv[])
{
	int limit = -1;
	int no_color = 0;
	int show_help = 0;

	const struct command_option options[] = {
			OPT_INT('n', "max-count", "limit number of messages shown", &limit),
			OPT_LONG_BOOL("no-color", "turn off colored message headers", &no_color),
			OPT_BOOL('h', "help", "show usage and exit", &show_help),
			OPT_END()
	};

	argc = parse_options(argc, argv, options, 1, 1);
	if (show_help) {
		show_usage_with_options(read_cmd_usage, options, 0, NULL);
		return 0;
	}

	if (argc > 1) {
		show_usage_with_options(read_cmd_usage, options, 1,
				"error: only one message may be read at a time.");
		return 1;
	}

	if (!is_inside_git_chat_space())
		DIE("Where are you? It doesn't look like you're in the right directory.");

	if (!isatty(STDOUT_FILENO))
		no_color = 1;

	return read_messages(argc ? argv[0] : NULL, limit, no_color);
}
