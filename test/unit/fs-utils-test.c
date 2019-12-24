#include <stdio.h>
#include <string.h>
#include <sys/stat.h>
#include <unistd.h>
#include <fcntl.h>

#include "test-lib.h"
#include "fs-utils.h"

TEST_DEFINE(copy_file_test)
{
	const char *filename = "test.txt";
	const char *copy_filename = "test1.txt";
	FILE *fp_src = NULL;
	FILE *fp_dest = NULL;
	struct strbuf buf;

	TEST_START() {
		strbuf_init(&buf);

		unlink(copy_filename);
		unlink(filename);

		// create a file 'test.txt' with body 'test.txt'
		fp_src = fopen(filename, "w");
		assert_nonnull_msg(fp_src, "failed to open %s for writing", filename);

		size_t len = strlen(filename);
		size_t bytes_written = fwrite(filename, sizeof(char), len, fp_src);
		assert_eq_msg(len, bytes_written, "expected %lu bytes written but only %d were written", len, bytes_written);

		fclose(fp_src);
		fp_src = NULL;

		// copy the file to a new destination
		mode_t mode = S_IRUSR | S_IWUSR;
		ssize_t ret = copy_file(copy_filename, filename, mode);
		assert_neq_msg(-1, ret, "copy file should not fail to open src or destination file");
		assert_eq_msg(len, ret, "unexpected number of bytes written to output file: %zd", ret);

		// check new file mode
		struct stat st;
		assert_zero_msg(stat(copy_filename, &st), "failed to stat %s", copy_filename);

		mode_t new_mode = st.st_mode & (S_IRWXU | S_IRWXG | S_IRWXO);
		assert_eq_msg(mode, new_mode, "incorrect file mode %lo (expected %lo)", new_mode, mode);
		assert_eq_msg(len, st.st_size, "incorrect file size %lu (expected %lu)", st.st_size, len);

		// check new file contents
		fp_dest = fopen(copy_filename, "r");
		assert_nonnull_msg(fp_dest, "failed to open %s for reading", copy_filename);

		strbuf_grow(&buf, len);
		size_t bytes_read = fread(buf.buff, sizeof(char), len, fp_dest);
		assert_eq_msg(len, bytes_read, "unexpected number of bytes read");

		assert_string_eq(filename, buf.buff);
	}

	if (fp_src != NULL)
		fclose(fp_src);
	if (fp_dest != NULL)
		fclose(fp_dest);

	unlink(copy_filename);
	unlink(filename);

	strbuf_release(&buf);

	TEST_END();
}

TEST_DEFINE(copy_file_unknown_file_test)
{
	const char *unknown_file = "unknown";
	const char *copy_unknown = "copy_unknown";
	FILE *fp_src = NULL;
	FILE *fp_dest = NULL;

	TEST_START() {
		unlink(unknown_file);
		unlink(copy_unknown);

		mode_t mode = S_IRUSR | S_IWUSR;
		ssize_t ret = copy_file(copy_unknown, unknown_file, mode);
		assert_eq_msg(-1, ret, "copy file should return -1 if src file %s does not exist", unknown_file);

		// create a file copy_unknown
		fp_src = fopen(unknown_file, "w");
		assert_nonnull_msg(fp_src, "failed to open %s for writing", unknown_file);

		size_t len = strlen(unknown_file);
		size_t bytes_written = fwrite(unknown_file, sizeof(char), len, fp_src);
		assert_eq_msg(len, bytes_written, "expected %lu bytes written but only %d were written", len, bytes_written);

		// create a file copy_unknown
		fp_dest = fopen(copy_unknown, "w");
		assert_nonnull_msg(fp_dest, "failed to open %s for writing", copy_unknown);

		len = strlen(copy_unknown);
		bytes_written = fwrite(copy_unknown, sizeof(char), len, fp_dest);
		assert_eq_msg(len, bytes_written, "expected %lu bytes written but only %d were written", len, bytes_written);

		// because this checks for O_EXCL, it should fail with -1
		ret = copy_file(copy_unknown, unknown_file, mode);
		assert_eq_msg(-1, ret, "copy file should return -1 if src file does not exist");
	}

	if (fp_src != NULL)
		fclose(fp_src);
	if (fp_dest != NULL)
		fclose(fp_dest);

	unlink(unknown_file);
	unlink(copy_unknown);

	TEST_END();
}

TEST_DEFINE(copy_file_same_src_dest_test)
{
	const char *filename = "test.txt";
	FILE *fp = NULL;

	TEST_START() {
		unlink(filename);

		mode_t mode = S_IRUSR | S_IWUSR;

		// create a file copy_unknown
		fp = fopen(filename, "w");
		assert_nonnull_msg(fp, "failed to open %s for writing", filename);

		size_t len = strlen(filename);
		size_t bytes_written = fwrite(filename, sizeof(char), len, fp);
		assert_eq_msg(len, bytes_written, "expected %lu bytes written but only %d were written", len, bytes_written);

		// should return -1 since input file and output file are the same
		ssize_t ret = copy_file(filename, filename, mode);
		assert_eq_msg(-1, ret, "copy_file should return -1 if src and dest files are the same");
	}

	if (fp != NULL)
		fclose(fp);

	unlink(filename);

	TEST_END();
}

TEST_DEFINE(copy_file_src_dest_not_file)
{
	const char *filename = "test.txt";
	const char *directory = "test";
	FILE *fp_src = NULL;

	TEST_START() {
		unlink(filename);
		rmdir(directory);

		mode_t mode = S_IRUSR | S_IWUSR;

		// create a file 'test.txt' with body 'test.txt'
		fp_src = fopen(filename, "w");
		assert_nonnull_msg(fp_src, "failed to open %s for writing", filename);

		size_t len = strlen(filename);
		size_t bytes_written = fwrite(filename, sizeof(char), len, fp_src);
		assert_eq_msg(len, bytes_written, "expected %lu bytes written but only %d were written", len, bytes_written);

		fclose(fp_src);
		fp_src = NULL;

		assert_eq_msg(0, mkdir(directory, mode), "failed to create directory");

		// copy the file to a new destination
		ssize_t ret = copy_file(directory, filename, mode);
		assert_eq_msg(-1, ret, "copy_file should return -1 if destination is a directory");
		ret = copy_file(filename, directory, mode);
		assert_eq_msg(-1, ret, "copy_file should return -1 if src is a directory");
	}

	if (fp_src != NULL)
		fclose(fp_src);

	unlink(filename);
	rmdir(directory);

	TEST_END();
}

TEST_DEFINE(copy_file_fd_same_descriptor_test)
{
	// test setup
	const char *filename = "test.txt";
	int fd = -1;
	FILE *fp = NULL;

	TEST_START() {
		unlink(filename);

		fp = fopen(filename, "w");
		assert_nonnull_msg(fp, "failed to open %s for writing", filename);

		size_t len = strlen(filename);
		size_t bytes_written = fwrite(filename, sizeof(char), len, fp);
		assert_eq_msg(len, bytes_written, "expected %lu bytes written but only %d were written", len, bytes_written);

		fclose(fp);
		fp = NULL;

		mode_t mode = S_IRUSR | S_IWUSR;
		fd = open(filename, O_RDONLY, mode);
		assert_true_msg(fd >= 0, "failed to open file %s for writing", filename);

		ssize_t ret = copy_file_fd(fd, fd);
		assert_eq_msg(-1, ret, "expected copy_file_fd to return -1 if src and dest are the same, but was %zd", ret);
	}

	unlink(filename);
	if (fp != NULL)
		fclose(fp);
	if (fd != -1)
		close(fd);

	TEST_END();
}

TEST_DEFINE(copy_file_fd_test)
{
	// test setup
	const char *filename_in = "test.txt";
	const char *filename_out = "test_copy.txt";
	int in_fd = -1;
	int out_fd = -1;
	FILE *fp = NULL;

	TEST_START() {
		unlink(filename_in);
		unlink(filename_out);

		fp = fopen(filename_in, "w");
		assert_nonnull_msg(fp, "failed to open %s for writing", filename_in);

		size_t len = strlen(filename_in);
		size_t bytes_written = fwrite(filename_in, sizeof(char), len, fp);
		assert_eq_msg(len, bytes_written, "expected %lu bytes written but only %d were written", len, bytes_written);

		fclose(fp);
		fp = NULL;

		in_fd = open(filename_in, O_RDONLY);
		assert_true_msg(in_fd >= 0, "failed to open file %s for reading", filename_in);

		mode_t mode = S_IRUSR | S_IWUSR;
		out_fd = open(filename_out, O_WRONLY | O_EXCL | O_CREAT, mode);
		assert_true_msg(out_fd >= 0, "failed to open file %s for writing", filename_out);

		ssize_t ret = copy_file_fd(out_fd, in_fd);
		assert_eq_msg(len, ret, "copy_file_fd should return the number of bytes copied, but was %zd", ret);
	}

	unlink(filename_in);
	unlink(filename_out);
	if (fp != NULL)
		fclose(fp);
	if (in_fd != -1)
		close(in_fd);
	if (out_fd != -1)
		close(out_fd);

	TEST_END();
}

TEST_DEFINE(get_symlink_target_not_symlink_test)
{
	// test setup
	const char *filename = "test.txt";
	FILE *fp = NULL;
	struct strbuf result;

	TEST_START() {
		unlink(filename);

		strbuf_init(&result);

		fp = fopen(filename, "w");
		assert_nonnull_msg(fp, "failed to open %s for writing", filename);

		size_t len = strlen(filename);
		size_t bytes_written = fwrite(filename, sizeof(char), len, fp);
		assert_eq_msg(len, bytes_written, "expected %lu bytes written but only %d were written", len, bytes_written);

		fclose(fp);
		fp = NULL;

		int ret = get_symlink_target(filename, &result, 0);
		assert_nonzero_msg(ret, "expected get_symlink_target to return nonzero, but was %d", ret);
	}

	unlink(filename);
	if (fp != NULL)
		fclose(fp);

	strbuf_release(&result);

	TEST_END();
}

TEST_DEFINE(get_symlink_target_test)
{
	// test setup
	const char *filename = "test.txt";
	const char *link_name = "test.txt.ln";
	FILE *fp = NULL;
	struct strbuf result;

	TEST_START() {
		unlink(filename);
		unlink(link_name);

		strbuf_init(&result);

		fp = fopen(filename, "w");
		assert_nonnull_msg(fp, "failed to open %s for writing", filename);

		size_t len = strlen(filename);
		size_t bytes_written = fwrite(filename, sizeof(char), len, fp);
		assert_eq_msg(len, bytes_written, "expected %lu bytes written but only %d were written", len, bytes_written);

		fclose(fp);
		fp = NULL;

		assert_zero_msg(symlink(filename, link_name), "failed to symlink %s to %s", filename, link_name);

		int ret = get_symlink_target(link_name, &result, 0);
		assert_zero_msg(ret, "expected get_symlink_target to return zero, but was %d", ret);

		assert_string_eq(filename, result.buff);
	}

	unlink(filename);
	unlink(link_name);
	if (fp != NULL)
		fclose(fp);

	strbuf_release(&result);

	TEST_END();
}

TEST_DEFINE(safe_create_dir_exists_test)
{
	const char *directory = "test";
	const char *filename = "test/test.txt";
	FILE *fp = NULL;

	TEST_START() {
		rmdir(directory);

		mode_t mode = S_IRWXU | S_IRWXG | S_IROTH | S_IXOTH;
		assert_eq_msg(0, mkdir(directory, mode), "failed to create directory");

		fp = fopen(filename, "w");
		assert_nonnull_msg(fp, "failed to open %s for writing", filename);

		size_t len = strlen(filename);
		size_t bytes_written = fwrite(filename, sizeof(char), len, fp);
		assert_eq_msg(len, bytes_written, "expected %lu bytes written but only %d were written", len, bytes_written);

		fclose(fp);
		fp = NULL;

		safe_create_dir(directory, NULL, mode);

		struct stat sb;
		assert_zero_msg(stat(directory, &sb), "failed to stat %s", directory);
		assert_true_msg(S_ISDIR(sb.st_mode), "%s is not a directory", directory);

		assert_zero_msg(stat(filename, &sb), "failed to stat %s", filename);
		assert_true_msg(S_ISREG(sb.st_mode), "%s is not a file", filename);
	}

	unlink(filename);
	if (fp != NULL)
		fclose(fp);
	rmdir(directory);

	TEST_END();
}

TEST_DEFINE(safe_create_dir_not_exists_test)
{
	const char *directory = "test";

	TEST_START() {
		rmdir(directory);

		mode_t mode = S_IRWXU | S_IRWXG | S_IROTH | S_IXOTH;
		safe_create_dir(directory, NULL, mode);

		struct stat sb;
		assert_zero_msg(stat(directory, &sb), "failed to stat %s", directory);
		assert_true_msg(S_ISDIR(sb.st_mode), "%s is not a directory", directory);
	}

	rmdir(directory);

	TEST_END();
}

TEST_DEFINE(find_in_path_test)
{
	const char *directory = "test";
	const char *filename = "test/test.txt";
	FILE *fp = NULL;
	struct strbuf tmp;
	char *path_to_file = NULL;
	char *original_path = getenv("PATH");

	TEST_START() {
		rmdir(directory);

		mode_t mode = S_IRWXU | S_IRWXG | S_IROTH | S_IXOTH;
		assert_eq_msg(0, mkdir(directory, mode), "failed to create directory");

		fp = fopen(filename, "w");
		assert_nonnull_msg(fp, "failed to open %s for writing", filename);

		size_t len = strlen(filename);
		size_t bytes_written = fwrite(filename, sizeof(char), len, fp);
		assert_eq_msg(len, bytes_written, "expected %lu bytes written but only %d were written", len, bytes_written);

		fclose(fp);
		fp = NULL;

		assert_zero_msg(chmod(filename, mode), "unable to set permission %lo for file %s", mode, filename);

		strbuf_init(&tmp);
		strbuf_attach_fmt(&tmp, "%s:", original_path);
		get_cwd(&tmp);
		strbuf_attach_fmt(&tmp, "/%s", directory);

		assert_zero_msg(setenv("PATH", tmp.buff, 1), "unable to set PATH environment variable");

		path_to_file = find_in_path("test.txt");
		assert_nonnull_msg(path_to_file, "find_in_path returned null unexpectedly");

		strbuf_release(&tmp);
		strbuf_init(&tmp);
		get_cwd(&tmp);
		strbuf_attach_fmt(&tmp, "/%s", filename);

		assert_string_eq(tmp.buff, path_to_file);
	}

	unlink(filename);
	if (fp != NULL)
		fclose(fp);
	rmdir(directory);
	strbuf_release(&tmp);
	if (path_to_file)
		free(path_to_file);

	setenv("PATH", original_path, 1);

	TEST_END();
}

TEST_DEFINE(is_executable_test)
{
	const char *filename = "test.txt";
	FILE *fp = NULL;

	TEST_START() {
		unlink(filename);
		mode_t mode = S_IRWXU | S_IRWXG | S_IROTH | S_IXOTH;

		fp = fopen(filename, "w");
		assert_nonnull_msg(fp, "failed to open %s for writing", filename);

		size_t len = strlen(filename);
		size_t bytes_written = fwrite(filename, sizeof(char), len, fp);
		assert_eq_msg(len, bytes_written, "expected %lu bytes written but only %d were written", len, bytes_written);

		fclose(fp);
		fp = NULL;

		assert_zero_msg(is_executable(filename), "file %s should not be executable", filename);

		assert_zero_msg(chmod(filename, mode), "unable to set permission %lo for file %s", mode, filename);
		assert_nonzero_msg(is_executable(filename), "file %s should not be executable", filename);
	}

	unlink(filename);
	if (fp != NULL)
		fclose(fp);

	TEST_END();
}

int fs_utils_test(struct test_runner_instance *instance)
{
	struct unit_test tests[] = {
			{ "copy_file should create a new file with identical file content", copy_file_test },
			{ "copy_file should return -1 if src or dest files cannot be opened", copy_file_unknown_file_test },
			{ "copy_file should return -1 if src and dest point to the same file", copy_file_same_src_dest_test },
			{ "copy_file should return -1 if src or dest are not files", copy_file_src_dest_not_file },
			{ "copy_file_fd should return -1 if src and dest are the same descriptor", copy_file_fd_same_descriptor_test },
			{ "copy_file_fd should return -1 if given the same src and destination descriptor", copy_file_same_src_dest_test },
			{ "copy_file_fd should return the number of bytes successfully copied", copy_file_fd_test },
			{ "get_symlink_target should return nonzero if target path is not symbolic link", get_symlink_target_not_symlink_test },
			{ "get_symlink_target should correctly determine the target of a symbolic link", get_symlink_target_test },
			{ "safe_create_dir should not create the directory if it exists", safe_create_dir_exists_test },
			{ "safe_create_dir should create the directory if it does not exist", safe_create_dir_not_exists_test },
			{ "find_in_path should correctly find an executable on the PATH", find_in_path_test },
			{ "is_executable should correctly determine if file is executable", is_executable_test },
			{ NULL, NULL }
	};

	return execute_tests(instance, tests);
}