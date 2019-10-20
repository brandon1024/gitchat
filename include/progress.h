#ifndef GIT_CHAT_PROGRESS_H
#define GIT_CHAT_PROGRESS_H

/**
 * progress api
 *
 * TODO
 * */

enum progress_anim_type {
	PROGRESS_PERCENT,
	PROGRESS_INDETERMINATE
};

struct progress_ctx {
	enum progress_anim_type type;
	unsigned long ticks;
	unsigned long total;
	const char *title;
};

/**
 *
 * */
void progress_ctx_init(struct progress_ctx *ctx, enum progress_anim_type type);

/**
 *
 * */
void progress_ctx_set_title(struct progress_ctx *ctx, const char *title);

/**
 *
 * */
void progress_ctx_set_ticks(struct progress_ctx *ctx, unsigned long ticks);

/**
 *
 * */
void progress_start(struct progress_ctx *ctx, unsigned long total);

/**
 *
 * */
void progress_increment(struct progress_ctx *ctx);

/**
 *
 * */
void progress_stop(struct progress_ctx *ctx, int complete);

#endif //GIT_CHAT_PROGRESS_H
