#include <stdio.h>
#include <signal.h>
#include <string.h>
#include <sys/time.h>
#include <sys/ioctl.h>
#include <unistd.h>

#include "progress.h"
#include "utils.h"

static void progress_update(struct progress_ctx *);
static void progress_update_type_percent(struct progress_ctx *);
static void progress_update_type_tick(struct progress_ctx *);
static void setup_update_signal(void);
static void clear_update_signal(void);
static void progress_update_handler(int);

static volatile sig_atomic_t update;

void progress_ctx_init(struct progress_ctx *ctx, enum progress_anim_type type)
{
	*ctx = (struct progress_ctx) {
		.type = type,
		.ticks = 0,
		.total = 0,
		.title = NULL
	};
}

void progress_ctx_set_title(struct progress_ctx *ctx, const char *title)
{
	ctx->title = title;
}

void progress_ctx_set_ticks(struct progress_ctx *ctx, unsigned long ticks)
{
	ctx->ticks = ticks;
}

void progress_start(struct progress_ctx *ctx, unsigned long total)
{
	ctx->total = total;
	setup_update_signal();
}

void progress_increment(struct progress_ctx *ctx)
{
	ctx->ticks++;
	progress_update(ctx);
}

void progress_stop(struct progress_ctx *ctx, int complete)
{
	clear_update_signal();
	if (complete)
		ctx->ticks = ctx->total;

	update = 1;
	progress_update(ctx);
	fprintf(stderr, "\n");
}

static void progress_update(struct progress_ctx *ctx)
{
	if (!update)
		return;

	switch (ctx->type) {
		case PROGRESS_PERCENT:
			progress_update_type_percent(ctx);
			break;
		case PROGRESS_INDETERMINATE:
			progress_update_type_tick(ctx);
			break;
		default:
			BUG("unknown progress_anim_type enumeration in progress_update()");
	}
}

static void progress_update_type_percent(struct progress_ctx *ctx)
{
	const char *title = ctx->title ? ctx->title : "";

	unsigned short int percent;
	if (ctx->ticks >= ctx->total)
		percent = 100;
	else
		percent = ctx->ticks * 100 / ctx->total;

	char progress_bar[50];
	memset(progress_bar, '.', 50);
	memset(progress_bar, '#', percent / 2);

	struct winsize ws;
	if (ioctl(STDERR_FILENO, TIOCGWINSZ, &ws) < 0)
		FATAL("failed to determine terminal character width for displaying progress");

	int spaces = (int)ws.ws_col - 58 - (int)strlen(title) - 1;
	if (spaces < 0)
		spaces = 1;
	fprintf(stderr, "\r%s%*s[%.*s] (%03hu%%)", title, spaces, " ", 50, progress_bar, percent);
	fflush(stderr);
}

static void progress_update_type_tick(struct progress_ctx *ctx)
{
	const char progress_chars[] = {'-', '\\', '|', '/'};

	const char *title = ctx->title ? ctx->title : "";
	fprintf(stderr, "\r%s %c", title,
			progress_chars[ctx->ticks % ARRAY_SIZE(progress_chars)]);
	fflush(stderr);
}

static void setup_update_signal(void)
{
	update = 0;

	struct sigaction sa;
	memset(&sa, 0, sizeof(sa));
	sa.sa_handler = progress_update_handler;
	sigemptyset(&sa.sa_mask);
	sa.sa_flags = SA_RESTART;
	sigaction(SIGALRM, &sa, NULL);

	// timer every 500ms
	struct itimerval v;
	v.it_interval.tv_sec = 0;
	v.it_interval.tv_usec = 500000;
	v.it_value = v.it_interval;
	setitimer(ITIMER_REAL, &v, NULL);
}

static void clear_update_signal(void)
{
	struct itimerval v = {{0,},};
	setitimer(ITIMER_REAL, &v, NULL);
	signal(SIGALRM, SIG_IGN);
}

static void progress_update_handler(int signum)
{
	update = 1;
}
