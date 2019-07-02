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

#define BUFF_LEN 1024

void copy_dir(char *path_from, char *path_to)
{
	DIR *dir;
	struct dirent *ent;

	dir = opendir(path_from);
	if (!dir)
		FATAL("unable to open directory '%s'", path_from);

	safe_create_dir(path_to, NULL);
	while ((ent = readdir(dir)) != NULL) {
		struct stat st_from;

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
			if (get_symlink_target(new_path_from.buff, &target, st_from.st_size))
				FATAL("unable to read symlink target for '%s'", new_path_from.buff);

			if (symlink(target.buff, new_path_to.buff))
				FATAL("cannot symlink '%s' to '%s'");

			strbuf_release(&target);
		} else if (S_ISREG(st_from.st_mode)) {
			copy_file(new_path_to.buff, new_path_from.buff, st_from.st_mode);
		} else
			DIE("cannot copy from '%s' to '%s'; unexpected file",
				new_path_from.buff, new_path_to.buff);

		strbuf_release(&new_path_from);
		strbuf_release(&new_path_to);
	}

	closedir(dir);
}

void copy_file(const char *dest, const char *src, int mode)
{
	int in_fd, out_fd;
	ssize_t bytes_read;
	char buffer[BUFF_LEN];

	if (!(in_fd = open(src, O_RDONLY)))
		FATAL(FILE_OPEN_FAILED, src);

	if (!(out_fd = open(dest, O_WRONLY | O_CREAT | O_EXCL, mode)))
		FATAL(FILE_OPEN_FAILED, dest);

	while ((bytes_read = read(in_fd, buffer, BUFF_LEN)) > 0) {
		if (write(out_fd, buffer, bytes_read) != bytes_read)
			FATAL(FILE_WRITE_FAILED, dest);
	}

	if (bytes_read < 0)
		FATAL("unexpected error while reading from file '%s'", src);

	LOG_TRACE("File copied from '%s' to '%s'", src, dest);

	close(in_fd);
	close(out_fd);
}

int get_symlink_target(char *symlink_path, struct strbuf *result, size_t size)
{
	if (size < 64)
		size = 64;

	while (size < PATH_MAX) {
		ssize_t len;

		strbuf_grow(result, size);
		len = readlink(symlink_path, result->buff, size);
		if (len < 0)
			return 1;

		if (len < size)
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

void safe_create_dir(char *base_path, char *dir)
{
	struct strbuf buff;
	strbuf_init(&buff);

	strbuf_attach(&buff, base_path, PATH_MAX);

	if (dir)
		strbuf_attach_fmt(&buff, "/%s", dir);

	if (mkdir(buff.buff, 0777) < 0) {
		if (errno != EEXIST)
			FATAL("unable to create directory '%s'", buff.buff);

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
