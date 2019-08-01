#!/usr/bin/env bash
#
# Integration test runner for all integration tests.
#
# Options:
#	--from-dir <directory>
#		By default, tests are executed from 'trash/' directory relative to this
#		script. This command is used to override this.
#
#	--git-chat-installed <path>
#		If git-chat is installed in an alternate directory, use that installation
#		rather than the global one. Simply places the given directory at the
#		beginning of the PATH.
#
#	--test <pattern>
#		Run specific tests according to a given pattern.
#
#	-v, --verbose
#		Be more verbose. Prints the output from the test setup and test commands
#		executed.
#
#	-d, --debug
#		Print very verbose debugging information. This will set -x on each
#		integration test, and will set the GIT_CHAT_LOG_LEVEL to TRACE. This
#		is particularly useful when debugging strange behavior in failing tests.
#
#	--no-color
#		Don't print colored output.
#
#
# Environment:
#	TEST_TRASH_DIR
#		Equivalent to using --from-dir <directory> option.
#
#	TEST_GIT_CHAT_INSTALLED
#		Equivalent to using --git-chat-installed <path> option.
#
#	TEST_PATTERN
#		Equivalent to using --test <pattern> option.
#
#	TEST_VERBOSE
#		Equivalent to using --verbose option.
#
#	TEST_DEBUG
#		Equivalent to using --debug option.
#
#	TEST_NO_COLOR_OUT
#		Equivalent to using -- option.
#

TEST_RUNNER_PATH="$( cd "$(dirname "$0")" ; pwd -P )"
TEST_TRASH_DIR="${TEST_TRASH_DIR:-}"
TEST_GIT_CHAT_INSTALLED="${TEST_GIT_CHAT_INSTALLED:-}"
TEST_PATTERN="${TEST_PATTERN:-t[0-9][0-9][0-9]*.sh}"
TEST_VERBOSE="${TEST_VERBOSE:-}"
TEST_DEBUG="${TEST_DEBUG:-}"
TEST_NO_COLOR_OUT="${TEST_NO_COLOR_OUT:-}"
while [[ $# -gt 0 ]]; do
	case "$1" in
		--from-dir)
			TEST_TRASH_DIR=$2
			shift
			shift
			;;
		--git-chat-installed)
			export TEST_GIT_CHAT_INSTALLED=$2
			shift
			shift
			;;
		--test)
			TEST_PATTERN=$2
			shift
			shift
			;;
		-v|--verbose)
			TEST_VERBOSE=1
			shift
			;;
		-d|--debug)
			TEST_DEBUG=1
			shift
			;;
		--no-color)
			TEST_NO_COLOR_OUT=1
			shift
			;;
		*)
			echo "Error: Unknown option $1" 1>&2
			exit 1
			;;
	esac
done

if [ ! -z "${TEST_DEBUG}" ]; then
	set -x
fi

debug () {
	if [ ! -z "${TEST_DEBUG}" ]; then
		echo 'DEBUG:' "${1}"
	fi
}

rebuild_trash_dir () {
	debug "rebuilding trash directory ${TEST_TRASH_DIR}"
	# clear/create trash directory
	if [ -d "$TEST_TRASH_DIR" ]; then
		rm -rf $TEST_TRASH_DIR
	fi

    mkdir $TEST_TRASH_DIR
}

#
# Test Start
#
if [ ! -z "${TEST_GIT_CHAT_INSTALLED}" ]; then
	export PATH="${TEST_GIT_CHAT_INSTALLED}:$PATH"
	debug "PATH: ${PATH}"
fi

debug "integration runner path: ${TEST_RUNNER_PATH}"

if [ -z "${TEST_TRASH_DIR}" ]; then
	TEST_TRASH_DIR="${TEST_RUNNER_PATH}/trash"
fi

debug "trash directory path: ${TEST_TRASH_DIR}"
debug "test pattern: ${TEST_PATTERN}"

# Trick Git into thinking that the test trash directory is not in a git working tree
GIT_CEILING_DIRECTORIES="$(dirname "$TEST_TRASH_DIR")"

if [ ! -d "${TEST_RUNNER_PATH}/integration" ]; then
	echo 'No tests were found; directory does not exist: $TEST_RUNNER_PATH/integration' 1>&2
fi

debug "running tests from path: ${TEST_RUNNER_PATH}/integration"
cd $TEST_RUNNER_PATH/integration

export TEST_TRASH_DIR
export TEST_VERBOSE
export TEST_DEBUG
export TEST_NO_COLOR_OUT
export TEST_TRASH_DIR
export GIT_CEILING_DIRECTORIES

TESTS="${TEST_RUNNER_PATH}/integration/${TEST_PATTERN}"
TEST_FAILURES=0
for test_path in $TESTS; do
	rebuild_trash_dir

	echo '***' $(basename -- "${test_path}") '***'
	bash $test_path

	if [[ ${?} -ne 0 ]]; then
		TEST_FAILURES=$((TEST_FAILURES + 1))
	fi

	echo
done

debug "finished running tests"

echo "Test execution completed with ${TEST_FAILURES} test failures."
if [[ ${TEST_FAILURES} -ne 0 ]]; then
	echo "Run with --debug for more details."
	exit 1
fi

exit 0
