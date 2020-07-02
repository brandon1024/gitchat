#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <limits.h>
#include <errno.h>
#include <sys/stat.h>
#include <dirent.h>
#include <fcntl.h>

#include "fs-utils.h"
#include "utils.h"

/* This is not a POSIX standard, so need to define it if it isn't defined */
#ifndef PATH_MAX
#define PATH_MAX 4096
#endif

#define BUFF_LEN 4096

void copy_dir(char *path_from, char *path_to)
{
	int errsv = errno;
	DIR *dir;
	struct dirent *ent;

	dir = opendir(path_from);
	if (!dir)
		FATAL("unable to open directory '%s'", path_from);

	struct stat st_from;
	if (lstat(path_from, &st_from) && errno == ENOENT)
		FATAL("unable to stat directory '%s'", path_from);

	safe_create_dir(path_to, NULL, st_from.st_mode);
	while ((ent = readdir(dir)) != NULL) {
		if (!strcmp(".", ent->d_name) || !strcmp("..", ent->d_name))
			continue;

		struct strbuf new_path_from;
		struct strbuf new_path_to;
		strbuf_init(&new_path_from);
		strbuf_init(&new_path_to);
		strbuf_attach_fmt(&new_path_from, "%s/%s", path_from, ent->d_name);
		strbuf_attach_fmt(&new_path_to, "%s/%s", path_to, ent->d_name);

		if (lstat(new_path_from.buff, &st_from) && errno == ENOENT)
			FATAL("unable to stat directory '%s'", new_path_from.buff);

		if (S_ISDIR(st_from.st_mode)) {
			//need to recurse.
			copy_dir(new_path_from.buff, new_path_to.buff);
		} else if (S_ISLNK(st_from.st_mode)) {
			//read the target of the symbolic link, then attempt to symlink.
			struct strbuf target;
			strbuf_init(&target);

			// note: the st_size of a symbolic link is the length of the
			// pathname it contains, without a terminating null byte. That's why
			// we pass st_from.st_size to get_symlink_target().
			if (get_symlink_target(new_path_from.buff, &target, st_from.st_size))
				FATAL("unable to read symlink target for '%s'", new_path_from.buff);

			if (symlink(target.buff, new_path_to.buff))
				FATAL("cannot symlink '%s' to '%s'");

			strbuf_release(&target);
		} else if (S_ISREG(st_from.st_mode)) {
			ssize_t bytes_written = copy_file(new_path_to.buff,
					new_path_from.buff, st_from.st_mode & (S_IRWXU | S_IRWXG | S_IRWXO));

			if (bytes_written < 0)
				FATAL("cannot copy from '%s' to '%s'; unable to open for src "
					"or dest file",new_path_from.buff, new_path_to.buff);
			if (bytes_written != st_from.st_size)
				FATAL("cannot copy from '%s' to '%s'; successfully copied %lu of %lu bytes",
						bytes_written, st_from.st_size);
		} else
			DIE("cannot copy from '%s' to '%s'; unexpected file",
				new_path_from.buff, new_path_to.buff);

		strbuf_release(&new_path_from);
		strbuf_release(&new_path_to);
	}

	closedir(dir);
	errno = errsv;
}

ssize_t copy_file(const char *dest, const char *src, mode_t mode)
{
	if (!strcmp(src, dest))
		return -1;

	int in_fd, out_fd;
	if ((in_fd = open(src, O_RDONLY)) < 0)
		return -1;
	if ((out_fd = open(dest, O_WRONLY | O_CREAT | O_EXCL, mode)) < 0)
		return -1;

	ssize_t bytes_written = copy_file_fd(out_fd, in_fd);
	close(in_fd);
	close(out_fd);

	LOG_TRACE("File copied from '%s' to '%s'", src, dest);

	return bytes_written;
}

ssize_t copy_file_fd(int dest_fd, int src_fd)
{
	if (src_fd == dest_fd)
		return -1;

	char buffer[BUFF_LEN];
	ssize_t bytes_written = 0;

	ssize_t bytes_read;
	while ((bytes_read = xread(src_fd, buffer, BUFF_LEN)) > 0) {
		// if write failed, return bytes_written
		if (xwrite(dest_fd, buffer, bytes_read) != bytes_read)
			return bytes_written;

		bytes_written += bytes_read;
	}

	if (bytes_read < 0)
		return bytes_read;

	return bytes_written;
}

int get_symlink_target(const char *symlink_path, struct strbuf *result, size_t size)
{
	if (size < 64)
		size = 64;

	while (size < PATH_MAX) {
		ssize_t len;

		strbuf_grow(result, size);
		len = readlink(symlink_path, result->buff, size);
		if (len < 0)
			return 1;

		if (len < (ssize_t)size)
			return 0;

		size *= 2;
	}

	return 1;
}

int get_cwd(struct strbuf *buff)
{
	char cwd[PATH_MAX];
	if (!getcwd(cwd, PATH_MAX))
		return 1;

	strbuf_attach(buff, cwd, PATH_MAX);

	return 0;
}

void safe_create_dir(const char *base_path, char *dir, unsigned int mode)
{
	int errsv = errno;
	struct strbuf buff;
	strbuf_init(&buff);

	strbuf_attach(&buff, base_path, PATH_MAX);

	if (dir)
		strbuf_attach_fmt(&buff, "/%s", dir);

	if (mkdir(buff.buff, mode) < 0) {
		if (errno != EEXIST)
			FATAL("unable to create directory '%s'", buff.buff);

		errno = errsv;
		LOG_WARN("Directory '%s' already exists", buff.buff);
	} else {
		LOG_TRACE("Created new directory '%s'", buff.buff);
	}

	strbuf_release(&buff);
}

char *find_in_path(const char *file)
{
	const char *p = getenv("PATH");
	struct strbuf buf;

	if (!p || !*p)
		return NULL;

	while (1) {
		const char *end = strchr(p, ':');
		strbuf_init(&buf);

		if (!end)
			end = strchr(p, 0);

		/* POSIX specifies an empty entry as the current directory. */
		if (end != p) {
			strbuf_attach(&buf, (char *)p, end - p);
			strbuf_attach_chr(&buf, '/');
		}

		strbuf_attach_str(&buf, (char *)file);

		if (is_executable(buf.buff))
			return strbuf_detach(&buf);

		strbuf_release(&buf);
		if (!*end)
			break;

		p = end + 1;
	}

	return NULL;
}

int is_executable(const char *name)
{
	struct stat st;
	int errsv = errno;

	int not_executable = stat(name, &st) || !S_ISREG(st.st_mode);
	errno = errsv;

	return not_executable ? 0 : st.st_mode & S_IXUSR;
}

void set_cloexec(int fd)
{
	int flags = fcntl(fd, F_GETFD);
	if (flags < 0 || fcntl(fd, F_SETFD, flags | FD_CLOEXEC) < 0)
		FATAL("fcntl() failed unexpectedly.");
}
