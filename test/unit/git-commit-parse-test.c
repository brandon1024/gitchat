#include "test-lib.h"
#include "git/git.h"
#include "git/commit.h"

#define HEADER_PREFIX_TREE "tree "
#define HEADER_PREFIX_PARENT "parent "
#define HEADER_PREFIX_AUTHOR "author "
#define HEADER_PREFIX_COMMITTER "committer "
#define HEADER_PREFIX_CUSTOM_1 "userdef1 "
#define HEADER_PREFIX_CUSTOM_2 "userdef2 "

#define OID_VALID "e4854A7f9bca6ac1bcaee3f1e8587f6953d542c0"
#define SIGNATURE_NAME "Brandon Richardson"
#define SIGNATURE_EMAIL "brandon.richardson@example.com"
#define SIGNATURE_TIMESTAMP "1602873674 -0300"

static int oid_empty(struct git_oid *oid)
{
	struct git_oid empty_oid;
	memset(empty_oid.id, 0, GIT_RAW_OBJECT_ID);

	return !memcmp(oid->id, empty_oid.id, GIT_RAW_OBJECT_ID);
}

static int oid_eq(struct git_oid *oid1, struct git_oid *oid2)
{
	return !memcmp(oid1->id, oid2->id, GIT_RAW_OBJECT_ID);
}

static int sig_empty(struct git_signature *sig)
{
	if (sig->name.len)
		return 0;
	if (sig->email.len)
		return 0;
	if (sig->timestamp.time)
		return 0;
	if (sig->timestamp.offset)
		return 0;

	return 1;
}

TEST_DEFINE(git_commit_object_init_test)
{
	struct git_commit commit;
	TEST_START() {
		git_commit_object_init(&commit);
		assert_true_msg(oid_empty(&commit.commit_id), "invalid commit id");
		assert_true_msg(oid_empty(&commit.tree_id), "invalid tree id");
		assert_null_msg(commit.parents_commit_ids,
				"expected null parents array");
		assert_zero_msg(commit.parents_commit_ids_len,
				"unexpected number of parents (expected 0)");
		assert_true_msg(sig_empty(&commit.author),
				"unexpected author signature defaults");
		assert_true_msg(sig_empty(&commit.committer),
				"unexpected committer signature defaults");
		assert_zero_msg(commit.body.len, "commit message body should be empty");
	}

	git_commit_object_release(&commit);

	TEST_END();
}

TEST_DEFINE(git_commit_object_release_test)
{
	struct git_commit commit;

	TEST_START() {
		git_commit_object_init(&commit);
		git_commit_object_release(&commit);

		assert_true_msg(oid_empty(&commit.commit_id), "invalid commit id");
		assert_true_msg(oid_empty(&commit.tree_id), "invalid tree id");
		assert_null_msg(commit.parents_commit_ids,
				"expected null parents array");
		assert_zero_msg(commit.parents_commit_ids_len,
				"unexpected number of parents (expected 0)");
	}

	TEST_END();
}

TEST_DEFINE(commit_parse_simple_test)
{
	struct git_commit commit;

	const char *commit_raw =
			HEADER_PREFIX_TREE OID_VALID "\n"
			HEADER_PREFIX_PARENT OID_VALID "\n"
			HEADER_PREFIX_AUTHOR SIGNATURE_NAME " <" SIGNATURE_EMAIL "> " SIGNATURE_TIMESTAMP "\n"
			HEADER_PREFIX_COMMITTER SIGNATURE_NAME " <" SIGNATURE_EMAIL "> " SIGNATURE_TIMESTAMP "\n"
			"\n"
			"    this is a test message\n";

	TEST_START() {
		git_commit_object_init(&commit);

		int ret = commit_parse(&commit, OID_VALID, commit_raw, strlen(commit_raw));
		assert_zero_msg(ret, "failed to parse commit");

		struct git_oid reference_oid;
		git_str_to_oid(&reference_oid, OID_VALID);

		assert_true_msg(oid_eq(&reference_oid, &commit.tree_id), "unexpected tree oid");

		assert_eq_msg(1, commit.parents_commit_ids_len, "unexpected number of parent oids");
		assert_true_msg(oid_eq(&reference_oid, &commit.parents_commit_ids[0]), "unexpected parent oid");

		assert_string_eq_msg(SIGNATURE_NAME, commit.author.name.buff, "unexpected author name");
		assert_string_eq_msg(SIGNATURE_EMAIL, commit.author.email.buff, "unexpected author email");
		assert_eq_msg((int64_t) 1602873674, commit.author.timestamp.time, "unexpected (unix) timestamp");
		assert_eq_msg(-180, commit.author.timestamp.offset, "unexpected timestamp offset");

		assert_string_eq_msg(SIGNATURE_NAME, commit.committer.name.buff, "unexpected author name");
		assert_string_eq_msg(SIGNATURE_EMAIL, commit.committer.email.buff, "unexpected author email");
		assert_eq_msg((int64_t) 1602873674, commit.committer.timestamp.time, "unexpected (unix) timestamp");
		assert_eq_msg(-180, commit.committer.timestamp.offset, "unexpected timestamp offset");

		assert_string_eq_msg("this is a test message", commit.body.buff, "unexpected commit message");
	}

	git_commit_object_release(&commit);

	TEST_END();
}

TEST_DEFINE(commit_parse_invalid_tree_test)
{
	struct git_commit commit;
	const char *commit_raw;
	int ret;

	TEST_START() {
		git_commit_object_init(&commit);

		commit_raw =
				HEADER_PREFIX_PARENT OID_VALID "\n"
				HEADER_PREFIX_AUTHOR SIGNATURE_NAME " <" SIGNATURE_EMAIL "> " SIGNATURE_TIMESTAMP "\n"
				HEADER_PREFIX_COMMITTER SIGNATURE_NAME " <" SIGNATURE_EMAIL "> " SIGNATURE_TIMESTAMP "\n"
				"\n"
				"    this is a test message\n";

		ret = commit_parse(&commit, OID_VALID, commit_raw, strlen(commit_raw));
		assert_nonzero_msg(ret, "expected commit_parse to fail with missing tree header");

		git_commit_object_release(&commit);
		git_commit_object_init(&commit);

		commit_raw =
				HEADER_PREFIX_TREE OID_VALID " \n"
				HEADER_PREFIX_PARENT OID_VALID "\n"
				HEADER_PREFIX_AUTHOR SIGNATURE_NAME " <" SIGNATURE_EMAIL "> " SIGNATURE_TIMESTAMP "\n"
				HEADER_PREFIX_COMMITTER SIGNATURE_NAME " <" SIGNATURE_EMAIL "> " SIGNATURE_TIMESTAMP "\n"
				"\n"
				"    this is a test message\n";

		ret = commit_parse(&commit, OID_VALID, commit_raw, strlen(commit_raw));
		assert_nonzero_msg(ret, "expected commit_parse to fail with unexpected whitespace after tree id");

		git_commit_object_release(&commit);
		git_commit_object_init(&commit);

		commit_raw =
				HEADER_PREFIX_TREE OID_VALID "\n"
				HEADER_PREFIX_TREE OID_VALID "\n"
				HEADER_PREFIX_PARENT OID_VALID "\n"
				HEADER_PREFIX_AUTHOR SIGNATURE_NAME " <" SIGNATURE_EMAIL "> " SIGNATURE_TIMESTAMP "\n"
				HEADER_PREFIX_COMMITTER SIGNATURE_NAME " <" SIGNATURE_EMAIL "> " SIGNATURE_TIMESTAMP "\n"
				"\n"
				"    this is a test message\n";

		ret = commit_parse(&commit, OID_VALID, commit_raw, strlen(commit_raw));
		assert_nonzero_msg(ret, "expected commit_parse to fail with multiple tree ids");
	}

	git_commit_object_release(&commit);

	TEST_END();
}

TEST_DEFINE(commit_parse_parents_test)
{
	struct git_commit commit;
	const char *commit_raw;
	int ret;

	TEST_START() {
		struct git_oid reference_oid;
		git_str_to_oid(&reference_oid, OID_VALID);

		git_commit_object_init(&commit);

		commit_raw =
				HEADER_PREFIX_TREE OID_VALID "\n"
				HEADER_PREFIX_PARENT OID_VALID "\n"
				HEADER_PREFIX_PARENT OID_VALID "\n"
				HEADER_PREFIX_AUTHOR SIGNATURE_NAME " <" SIGNATURE_EMAIL "> " SIGNATURE_TIMESTAMP "\n"
				HEADER_PREFIX_COMMITTER SIGNATURE_NAME " <" SIGNATURE_EMAIL "> " SIGNATURE_TIMESTAMP "\n"
				"\n"
				"    this is a test message\n";

		ret = commit_parse(&commit, OID_VALID, commit_raw, strlen(commit_raw));
		assert_zero_msg(ret, "expected commit_parse to pass with multiple parent commit ids");

		assert_eq_msg(2, commit.parents_commit_ids_len, "unexpected number of parent oids (expected 2)");
		assert_true_msg(oid_eq(&reference_oid, &commit.parents_commit_ids[0]), "unexpected parent oid");
		assert_true_msg(oid_eq(&reference_oid, &commit.parents_commit_ids[1]), "unexpected parent oid");

		git_commit_object_release(&commit);
		git_commit_object_init(&commit);

		commit_raw =
				HEADER_PREFIX_TREE OID_VALID "\n"
				HEADER_PREFIX_AUTHOR SIGNATURE_NAME " <" SIGNATURE_EMAIL "> " SIGNATURE_TIMESTAMP "\n"
				HEADER_PREFIX_COMMITTER SIGNATURE_NAME " <" SIGNATURE_EMAIL "> " SIGNATURE_TIMESTAMP "\n"
				"\n"
				"    this is a test message\n";

		ret = commit_parse(&commit, OID_VALID, commit_raw, strlen(commit_raw));
		assert_zero_msg(ret, "expected commit_parse to pass with no parent commit ids");

		assert_eq_msg(0, commit.parents_commit_ids_len, "unexpected number of parent oids (expected 0)");

		git_commit_object_release(&commit);
	}

	git_commit_object_release(&commit);

	TEST_END();
}

TEST_DEFINE(commit_parse_invalid_signature_test)
{
	struct git_commit commit;
	const char *commit_raw;
	int ret;

	TEST_START() {
		struct git_oid reference_oid;
		git_str_to_oid(&reference_oid, OID_VALID);

		git_commit_object_init(&commit);

		commit_raw =
				HEADER_PREFIX_TREE OID_VALID "\n"
				HEADER_PREFIX_PARENT OID_VALID "\n"
				HEADER_PREFIX_COMMITTER SIGNATURE_NAME " <" SIGNATURE_EMAIL "> " SIGNATURE_TIMESTAMP "\n"
				"\n"
				"    this is a test message\n";

		ret = commit_parse(&commit, OID_VALID, commit_raw, strlen(commit_raw));
		assert_nonzero_msg(ret, "expected commit_parse to fail with no author");

		git_commit_object_release(&commit);
		git_commit_object_init(&commit);

		commit_raw =
				HEADER_PREFIX_TREE OID_VALID "\n"
				HEADER_PREFIX_PARENT OID_VALID "\n"
				HEADER_PREFIX_AUTHOR SIGNATURE_NAME " <" SIGNATURE_EMAIL "> " SIGNATURE_TIMESTAMP "\n"
				"\n"
				"    this is a test message\n";

		ret = commit_parse(&commit, OID_VALID, commit_raw, strlen(commit_raw));
		assert_nonzero_msg(ret, "expected commit_parse to fail with no committer");

		git_commit_object_release(&commit);
		git_commit_object_init(&commit);

		commit_raw =
				HEADER_PREFIX_TREE OID_VALID "\n"
				HEADER_PREFIX_PARENT OID_VALID "\n"
				HEADER_PREFIX_AUTHOR SIGNATURE_NAME " " SIGNATURE_TIMESTAMP "\n"
				HEADER_PREFIX_COMMITTER SIGNATURE_NAME " <" SIGNATURE_EMAIL "> " SIGNATURE_TIMESTAMP "\n"
				"\n"
				"    this is a test message\n";

		ret = commit_parse(&commit, OID_VALID, commit_raw, strlen(commit_raw));
		assert_nonzero_msg(ret, "expected commit_parse to fail with no email");

		git_commit_object_release(&commit);
		git_commit_object_init(&commit);

		commit_raw =
				HEADER_PREFIX_TREE OID_VALID "\n"
				HEADER_PREFIX_PARENT OID_VALID "\n"
				HEADER_PREFIX_AUTHOR SIGNATURE_NAME " <" SIGNATURE_EMAIL "< " SIGNATURE_TIMESTAMP "\n"
				HEADER_PREFIX_COMMITTER SIGNATURE_NAME " <" SIGNATURE_EMAIL "> " SIGNATURE_TIMESTAMP "\n"
				"\n"
				"    this is a test message\n";

		ret = commit_parse(&commit, OID_VALID, commit_raw, strlen(commit_raw));
		assert_nonzero_msg(ret, "expected commit_parse to fail with incorrect placing of email delimiters '<>'");

		git_commit_object_release(&commit);
		git_commit_object_init(&commit);

		commit_raw =
				HEADER_PREFIX_TREE OID_VALID "\n"
				HEADER_PREFIX_PARENT OID_VALID "\n"
				HEADER_PREFIX_AUTHOR SIGNATURE_NAME " >" SIGNATURE_EMAIL "< " SIGNATURE_TIMESTAMP "\n"
				HEADER_PREFIX_COMMITTER SIGNATURE_NAME " <" SIGNATURE_EMAIL "> " SIGNATURE_TIMESTAMP "\n"
				"\n"
				"    this is a test message\n";

		ret = commit_parse(&commit, OID_VALID, commit_raw, strlen(commit_raw));
		assert_nonzero_msg(ret, "expected commit_parse to fail with incorrect placing of email delimiters '<>'");
	}

	git_commit_object_release(&commit);

	TEST_END();
}

TEST_DEFINE(commit_parse_valid_signature_test)
{
	struct git_commit commit;
	const char *commit_raw;
	int ret;

	TEST_START() {
		struct git_oid reference_oid;
		git_str_to_oid(&reference_oid, OID_VALID);

		git_commit_object_init(&commit);

		commit_raw =
				HEADER_PREFIX_TREE OID_VALID "\n"
				HEADER_PREFIX_PARENT OID_VALID "\n"
				HEADER_PREFIX_AUTHOR " <" SIGNATURE_EMAIL "> " SIGNATURE_TIMESTAMP "\n"
				HEADER_PREFIX_COMMITTER SIGNATURE_NAME " <" SIGNATURE_EMAIL "> " SIGNATURE_TIMESTAMP "\n"
				"\n"
				"    this is a test message\n";

		ret = commit_parse(&commit, OID_VALID, commit_raw, strlen(commit_raw));
		assert_zero_msg(ret, "expected commit_parse to pass with no author name");
		assert_string_eq_msg("", commit.author.name.buff, "expected author name to be empty");

		git_commit_object_release(&commit);
		git_commit_object_init(&commit);

		commit_raw =
				HEADER_PREFIX_TREE OID_VALID "\n"
				HEADER_PREFIX_PARENT OID_VALID "\n"
				HEADER_PREFIX_AUTHOR SIGNATURE_NAME " <" SIGNATURE_EMAIL "> " SIGNATURE_TIMESTAMP "\n"
				HEADER_PREFIX_AUTHOR SIGNATURE_NAME " <" SIGNATURE_EMAIL "> " SIGNATURE_TIMESTAMP "\n"
				HEADER_PREFIX_AUTHOR SIGNATURE_NAME " <" SIGNATURE_EMAIL "> " SIGNATURE_TIMESTAMP "\n"
				HEADER_PREFIX_COMMITTER SIGNATURE_NAME " <" SIGNATURE_EMAIL "> " SIGNATURE_TIMESTAMP "\n"
				"\n"
				"    this is a test message\n";

		ret = commit_parse(&commit, OID_VALID, commit_raw, strlen(commit_raw));
		assert_zero_msg(ret, "expected commit_parse to pass when given multiple authors");

		git_commit_object_release(&commit);
		git_commit_object_init(&commit);

		commit_raw =
				HEADER_PREFIX_TREE OID_VALID "\n"
				HEADER_PREFIX_PARENT OID_VALID "\n"
				HEADER_PREFIX_AUTHOR SIGNATURE_NAME " <" SIGNATURE_EMAIL "> " SIGNATURE_TIMESTAMP "\n"
				HEADER_PREFIX_COMMITTER SIGNATURE_NAME " <" SIGNATURE_EMAIL "> " SIGNATURE_TIMESTAMP "\n"
				HEADER_PREFIX_COMMITTER SIGNATURE_NAME " <" SIGNATURE_EMAIL "> " SIGNATURE_TIMESTAMP "\n"
				HEADER_PREFIX_COMMITTER SIGNATURE_NAME " <" SIGNATURE_EMAIL "> " SIGNATURE_TIMESTAMP "\n"
				"\n"
				"    this is a test message\n";

		ret = commit_parse(&commit, OID_VALID, commit_raw, strlen(commit_raw));
		assert_zero_msg(ret, "expected commit_parse to pass when given multiple committers");

		git_commit_object_release(&commit);
		git_commit_object_init(&commit);

		commit_raw =
				HEADER_PREFIX_TREE OID_VALID "\n"
				HEADER_PREFIX_PARENT OID_VALID "\n"
				HEADER_PREFIX_AUTHOR "   a     <> " SIGNATURE_TIMESTAMP "\n"
				HEADER_PREFIX_COMMITTER SIGNATURE_NAME " <  " SIGNATURE_EMAIL "  > " SIGNATURE_TIMESTAMP "\n"
				"\n"
				"    this is a test message\n";

		ret = commit_parse(&commit, OID_VALID, commit_raw, strlen(commit_raw));
		assert_zero_msg(ret, "expected commit_parse to pass when given multiple committers");
		assert_string_eq_msg("a", commit.author.name.buff, "author name should be trimmed of leading and trailing whitespace");
		assert_string_eq_msg("", commit.author.email.buff, "author email should be empty");
		assert_string_eq_msg(SIGNATURE_EMAIL, commit.committer.email.buff, "committer email should be trimmed of leading and trailing whitespace");
	}

	git_commit_object_release(&commit);

	TEST_END();
}

TEST_DEFINE(commit_parse_signature_missing_timestamp_test)
{
	struct git_commit commit;
	const char *commit_raw;
	int ret;

	TEST_START() {
		struct git_oid reference_oid;
		git_str_to_oid(&reference_oid, OID_VALID);

		git_commit_object_init(&commit);

		commit_raw =
				HEADER_PREFIX_TREE OID_VALID "\n"
				HEADER_PREFIX_PARENT OID_VALID "\n"
				HEADER_PREFIX_AUTHOR SIGNATURE_NAME " <" SIGNATURE_EMAIL ">\n"
				HEADER_PREFIX_COMMITTER SIGNATURE_NAME " <" SIGNATURE_EMAIL ">   \n"
				"\n"
				"    this is a test message\n";

		ret = commit_parse(&commit, OID_VALID, commit_raw, strlen(commit_raw));
		assert_zero_msg(ret, "expected commit_parse to pass with no signature timestamp");
		assert_eq_msg(0, commit.author.timestamp.time, "expected author timestamp to be 0");
		assert_eq_msg(0, commit.author.timestamp.offset, "expected author timestamp offset to be 0");

		git_commit_object_release(&commit);
		git_commit_object_init(&commit);

		commit_raw =
				HEADER_PREFIX_TREE OID_VALID "\n"
				HEADER_PREFIX_PARENT OID_VALID "\n"
				HEADER_PREFIX_AUTHOR SIGNATURE_NAME " <" SIGNATURE_EMAIL "> 1602873674\n"
				HEADER_PREFIX_COMMITTER SIGNATURE_NAME " <" SIGNATURE_EMAIL "> " SIGNATURE_TIMESTAMP "\n"
				"\n"
				"    this is a test message\n";

		ret = commit_parse(&commit, OID_VALID, commit_raw, strlen(commit_raw));
		assert_zero_msg(ret, "expected commit_parse to pass with no signature timestamp");
		assert_eq_msg((int64_t) 1602873674, commit.author.timestamp.time, "expected author timestamp to be 1602873674");
		assert_eq_msg(0, commit.author.timestamp.offset, "expected author timestamp offset to be 0");
	}

	git_commit_object_release(&commit);

	TEST_END();
}

TEST_DEFINE(commit_parse_skip_unknown_headers_test)
{
	struct git_commit commit;
	const char *commit_raw =
			HEADER_PREFIX_TREE OID_VALID "\n"
			HEADER_PREFIX_PARENT OID_VALID "\n"
			HEADER_PREFIX_AUTHOR SIGNATURE_NAME " <" SIGNATURE_EMAIL "> " SIGNATURE_TIMESTAMP "\n"
			HEADER_PREFIX_COMMITTER SIGNATURE_NAME " <" SIGNATURE_EMAIL "> " SIGNATURE_TIMESTAMP "\n"
			"custom single line\n"
			"gpgsig -----BEGIN PGP SIGNATURE-----\n"
			" \n"
			" iQEzBAABCgAdFiEEFmDqcrEirJTRbBcbcTZsRlJ0mZcFAl+LK8IACgkQcTZsRlJ0\n"
			" mZeRwgf/UyQAq6sOEchHtzUtXfs5+IGZ4HSxsj5wa2vZ5sVe1DV/9dQucryoJ+Ww\n"
			" wr2z4oeWG2ipIIohRn0jcjd8LxYqnNQ/gJWN0RqpjuXKBmxd8KSSwpF5ZFbtHb28\n"
			" +RBr1kEXNWzrYQIKpifLmEwMH5I155rPaepYmRE5NoHXStowu6jVNV/qlbPc8lyO\n"
			" 6hBpVs3Z41hRdUH8erIwx/gLSCVvzauF3jMFMRdBxfb+2CklVM1VKh9o09kuebhE\n"
			" 1lrRstxqe9DuqUNoPptfZPMbNTFtEZ5zJV5NMZsL+Dvhb8Ob8S0g7Z+bZqbu08GE\n"
			" hmv083qBrJJax35jJZT8eaX/cXD5Fw==\n"
			" =hrVU\n"
			" -----END PGP SIGNATURE-----\n"
			"\n"
			"this is a test message\n";

	TEST_START() {
		git_commit_object_init(&commit);

		int ret = commit_parse(&commit, OID_VALID, commit_raw, strlen(commit_raw));
		assert_zero_msg(ret, "failed to parse commit");

		assert_string_eq_msg("this is a test message", commit.body.buff, "unexpected commit message");
	}

	git_commit_object_release(&commit);

	TEST_END();
}

const char *suite_name = SUITE_NAME;
int test_suite(struct test_runner_instance *instance)
{
	struct unit_test tests[] = {
			{ "git_commit should initialize correctly", git_commit_object_init_test },
			{ "git_commit should release correctly", git_commit_object_release_test },
			{ "commit_parse should correctly parse simple commit object", commit_parse_simple_test },
			{ "commit_parse should fail if tree id missing", commit_parse_invalid_tree_test },
			{ "commit_parse should successfully parse parent commit ids", commit_parse_parents_test },
			{ "commit_parse should fail when supplied invalid signature", commit_parse_invalid_signature_test },
			{ "commit_parse should pass when supplied valid signature", commit_parse_valid_signature_test },
			{ "commit_parse should pass when supplied signature without timestamp", commit_parse_signature_missing_timestamp_test },
			{ "commit_parse should ignore unrecognized headers", commit_parse_skip_unknown_headers_test },
			{NULL, NULL}
	};

	return execute_tests(instance, tests);
}
