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
	struct _gpgme_key *key;
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
 * Set the gnupg home directory on a gpgme context.
 *
 * If home_dir is NULL, uses default home directory. Otherwise uses the given
 * home directory.
 * */
void gpgme_context_set_homedir(struct gc_gpgme_ctx *ctx, const char *home_dir);

/**
 * Release any resources under a git-chat gpgme context.
 * */
void gpg_context_release(struct gc_gpgme_ctx *ctx);


#endif //GIT_CHAT_GPG_COMMON_H
