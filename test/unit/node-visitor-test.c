#include "test-lib.h"
#include "config/node-visitor.h"

TEST_DEFINE(node_visitor_first_invocation_returns_root_test)
{
	struct config_data *conf;
	struct cd_node_visitor *visitor;

	TEST_START() {
		config_data_init(&conf);
		node_visitor_init(&visitor, conf);

		struct config_data *next;
		int status = node_visitor_next(visitor, &next);
		assert_zero_msg(status, "expected return value of 0 for first invocation of node_visitor_next");
		assert_eq_msg(conf, next, "node_visitor_next should return root node when first invoked");
	}

	node_visitor_release(visitor);
	config_data_release(&conf);

	TEST_END();
}

TEST_DEFINE(node_visitor_preorder_traveral_test)
{
	struct config_data *conf;
	struct cd_node_visitor *visitor;

	TEST_START() {
		config_data_init(&conf);
		node_visitor_init(&visitor, conf);

		assert_zero(config_data_insert(conf, "rootprop", "val"));
		assert_zero(config_data_insert(conf, "section.prop", "val"));
		assert_zero(config_data_insert(conf, "section.subsection.prop", "val"));
		assert_zero(config_data_insert(conf, "mysection.prop", "val"));
		assert_zero(config_data_insert(conf, "rootprop2", "val"));
		assert_zero(config_data_insert(conf, "section.subsection.prop2", "val"));
		assert_zero(config_data_insert(conf, "section.prop2", "val"));
		assert_zero(config_data_insert(conf, "section.prop3", "val"));

		struct config_data *next;
		int status = node_visitor_next(visitor, &next);
		assert_zero_msg(status, "unexpected return value from node_visitor_next");
		assert_null_msg(next->section, "root section name should be NULL");
		assert_eq_msg(2, next->entries.len, "unexpected number of root node properties");

		status = node_visitor_next(visitor, &next);
		assert_zero_msg(status, "unexpected return value from node_visitor_next");
		assert_string_eq("section", next->section);
		assert_eq_msg(3, next->entries.len, "unexpected number of root node properties");

		status = node_visitor_next(visitor, &next);
		assert_zero_msg(status, "unexpected return value from node_visitor_next");
		assert_string_eq("subsection", next->section);
		assert_eq_msg(2, next->entries.len, "unexpected number of root node properties");

		status = node_visitor_next(visitor, &next);
		assert_zero_msg(status, "unexpected return value from node_visitor_next");
		assert_string_eq("mysection", next->section);
		assert_eq_msg(1, next->entries.len, "unexpected number of root node properties");

		status = node_visitor_next(visitor, &next);
		assert_nonzero_msg(status, "node_visitor_next should return nonzero when traversal done");
	}

	node_visitor_release(visitor);
	config_data_release(&conf);

	TEST_END();
}

const char *suite_name = SUITE_NAME;
int test_suite(struct test_runner_instance *instance)
{
	struct unit_test tests[] = {
			{ "node_visitor_next should return root node when first invoked", node_visitor_first_invocation_returns_root_test },
			{ "node visitor should traverse config tree in preorder", node_visitor_preorder_traveral_test },
			{ NULL, NULL }
	};

	return execute_tests(instance, tests);
}
