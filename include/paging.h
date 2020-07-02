#ifndef GIT_CHAT_PAGING_H
#define GIT_CHAT_PAGING_H

/**
 * paging api - the git-chat pager
 *
 * The paging api is a utility that allows git-chat to pipe all standard output
 * to a paging application (like less, more, or cat) configured by the user.
 *
 * When paging, git-chat is in a volatile state. Any computation that is being
 * carried out cannot be guaranteed to finish since it's possible that the
 * user terminates the paging application, which will in turn halt the git-chat
 * runtime. With that said, the pager is particularly useful when displaying
 * information to the user, like message history, which doesn't rely on the
 * completion of computational work.
 *
 * When paging is enabled, the git-chat standard output streams (stdout, stderr)
 * are piped to the paging application, which is running as a child process. This
 * means the caller is free to use write(), printf(), puts(), etc., and the output
 * will be piped to the pager.
 *
 * The user can select which paging application is used through the following
 * environment variables, defined in order of precedence:
 * - GIT_CHAT_PAGER
 * - GIT_PAGER
 * - PAGER
 *
 * If any of those pagers are not available (i.e. cannot be found, or not executable),
 * then git-chat will default to "less", "more", or in dire cases, "cat".
 *
 * paging usage:
 *
 * int main(void) {
 *     // initializes the pager
 *     pager_start();
 *
 *     printf("large string... is paged");
 *     fprintf(stderr, "this is also paged");
 *     puts("so is this!");
 *     write(1, "and this too", 13);
 *
 *     // stops the pager, restoring standard streams
 *     return 0;
 * }
 * */

/**
 * Initialize the pager.
 *
 * Standard output stream must by a TTY. If the underlying paging application
 * terminates, git-chat will also exit.
 * */
void pager_start(void);

#endif //GIT_CHAT_PAGING_H
