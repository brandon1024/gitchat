#ifndef GIT_CHAT_GPG_COMMON_H
#define GIT_CHAT_GPG_COMMON_H

#include <gpgme.h>

#include "strbuf.h"

struct gc_gpgme_ctx {
	struct gpgme_context *gpgme_ctx;
	struct strbuf gnupg_homedir;
};

struct gpg_key_list {
	struct gpg_key_list_node *head;
	struct gpg_key_list_node *tail;
};

struct gpg_key_list_node {
	gpgme_key_t key;
	struct gpg_key_list_node *next;
	struct gpg_key_list_node *prev;
};

#include "key-manager.h"
#include "key-filter.h"
#include "encryption.h"
#include "decryption.h"
#include "utils.h"

/**
 * Retrieve a statically-allocated string representing the version of the gpgme
 * library.
 * */
const char *get_gpgme_library_version();

/**
 * Wrapper for FATAL(...) from utils.h that includes useful GPG error information.
 * */
NORETURN void GPG_FATAL(const char *msg, gpgme_error_t err);

/**
 * Initialize the gpgme OpenPGP engine.
 *
 * The gpgme engine is initialized under the GPGME_PROTOCOL_OpenPGP protocol.
 *
 * The gpgme engine is initialized using the default gpg executable and configuration
 * directory. To use an alternate gpg homedir, set the homedir on newly created
 * gpgme contexts.
 * */
void init_gpgme_openpgp_engine(void);

/**
 * Initialize a gpgme context. If use_gc_gpg_homedir is true, uses the git-chat
 * gpg homedir instead of the default one.
 * */
void gpgme_context_init(struct gc_gpgme_ctx *ctx, int use_gc_gpg_homedir);

/**
 * Release any resources under a git-chat gpgme context.
 * */
void gpgme_context_release(struct gc_gpgme_ctx *ctx);

/**
 * Configure the GPG engine with LOOPBACK pinentry mode and register a callback
 * function to supply the engine with a passphrase. Note that existing contexts
 * will not inherit this configuration, only those contexts created after this
 * function is called.
 *
 * The data pointed to by `cb_data` must still exist at the time the callback is
 * invoked (careful when using stack-allocated data).
 * */
void gpgme_configure_passphrase_loopback(gpgme_passphrase_cb_t cb, void *cb_data);

/**
 * Set the gnupg home directory on a gpgme context.
 *
 * If home_dir is NULL, uses default home directory. Otherwise uses the given
 * home directory.
 * */
void gpgme_context_set_homedir(struct gc_gpgme_ctx *ctx, const char *home_dir);

/**
 * GPG pinentry loopback callback function which reads passphrase from a given
 * file descriptor.
 * */
gpgme_error_t gpgme_pass_fd_cb(void *hook, const char *uid_hint,
		const char *passphrase_info, int prev_was_bad, int fd);

/**
 * GPG pinentry loopback callback function which reads passphrase from a file
 * at a given path.
 * */
gpgme_error_t gpgme_pass_file_cb(void *hook, const char *uid_hint,
		const char *passphrase_info, int prev_was_bad, int fd);

/**
 * GPG pinentry loopback callback function which uses passphrase given directly.
 * */
gpgme_error_t gpgme_pass_cb(void *hook, const char *uid_hint,
		const char *passphrase_info, int prev_was_bad, int fd);

#endif //GIT_CHAT_GPG_COMMON_H
