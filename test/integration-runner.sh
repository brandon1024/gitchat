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
#		Be more verbose.
#
#	-d, --debug
#		Print verbose debugging information.
#
#	--no-color
#		Don't print colored output.

TEST_TRASH_DIR=
TEST_PATTERN='t[0-9][0-9][0-9]*.sh'
TEST_VERBOSE=0
TEST_DEBUG=0
TEST_WITH_COLOR_OUT=1
while [[ $# -gt 0 ]]; do
	case "$1" in
		--from-dir)
			TEST_TRASH_DIR=$2
			shift
			shift
			;;
		--git-chat-installed)
			export PATH=$1:$PATH
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
			TEST_WITH_COLOR_OUT=0
			shift
			;;
		*)
			echo "Error: Unknown option $1" 1>&2
			exit 1
			;;
	esac
done

debug () {
	if [ "$TEST_DEBUG" -eq "1" ]; then
		echo 'DEBUG:' "$1"
	fi
}

rebuild_trash_dir () {
	debug "rebuilding trash directory $TEST_TRASH_DIR"
	# clear/create trash directory
	if [ -d "$TEST_TRASH_DIR" ]; then
		rm -rf $TEST_TRASH_DIR
	fi

    mkdir $TEST_TRASH_DIR
}

TEST_RUNNER_PATH="$( cd "$(dirname "$0")" ; pwd -P )"
debug "integration runner path: $TEST_RUNNER_PATH"

if [ -z ${TEST_TRASH_DIR} ]; then
	TEST_TRASH_DIR="$TEST_RUNNER_PATH/trash"
fi

debug "trash directory path: $TEST_TRASH_DIR"
debug "test pattern: $TEST_PATTERN"

# Trick Git into thinking that the test trash directory is not in a git working tree
GIT_CEILING_DIRECTORIES="$(dirname "$TEST_TRASH_DIR")"

TEST_RESOURCES_DIR="$TEST_RUNNER_PATH/resources"
debug "resources directory path: $TEST_RESOURCES_DIR"

if [ ! -d $TEST_RUNNER_PATH/integration ]; then
	echo "No tests were found; directory does not exist: $TEST_RUNNER_PATH/integration" 1>&2
fi

debug "running tests from path: $TEST_RUNNER_PATH/integration"
cd $TEST_RUNNER_PATH/integration

export TEST_TRASH_DIR
export TEST_VERBOSE
export TEST_DEBUG
export TEST_WITH_COLOR_OUT
export TEST_TRASH_DIR
export GIT_CEILING_DIRECTORIES
export TEST_RESOURCES_DIR

TESTS="$TEST_RUNNER_PATH/integration/$TEST_PATTERN"
TEST_FAILURES=0
for test_path in $TESTS; do
	rebuild_trash_dir

	if [[ ! -x "$test_path" ]]; then
		echo "Integration test file is not executable: $test_path" 1>&2
		exit 1
	fi

	echo '***' $(basename -- "$test_path") '***'
	sh $test_path

	if [ "$?" -ne "0" ]; then
		TEST_FAILURES=$((TEST_FAILURES + 1))
	fi

	echo
done

debug "finished running tests"

echo "Test execution completed with $TEST_FAILURES test failures."
if [ "$TEST_FAILURES" -ne "0" ]; then
	echo "Run with --debug for more details."
	exit 1
fi

exit 0