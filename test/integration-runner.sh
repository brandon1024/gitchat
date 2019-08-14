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
#	--valgrind
#		Run all git-chat binaries under valgrind memcheck. Note that this will
#		take much longer to execute.
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
#	TEST_VALGRIND
#		Equivalent to using --valgrind
#
#	VALGRIND_TOOL_OPTIONS
#		Pass options to valgrind memcheck. This will override any default options.
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

TEST_RUNNER_PATH="$( cd "$(dirname "${0}")" ; pwd -P )"
TEST_PATTERN="${TEST_PATTERN:-t[0-9][0-9][0-9]*.sh}"

while [[ ${#} -gt 0 ]]; do
	case "${1}" in
		--from-dir)
			TEST_TRASH_DIR=${2}
			shift
			shift
			;;
		--git-chat-installed)
			export TEST_GIT_CHAT_INSTALLED=${2}
			shift
			shift
			;;
		--test)
			TEST_PATTERN=${2}
			shift
			shift
			;;
		--valgrind)
			TEST_VALGRIND=1
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
			echo "Error: Unknown option ${1}" 1>&2
			exit 1
			;;
	esac
done

debug () {
	if [[ -n "${TEST_DEBUG+x}" ]]; then
		echo 'DEBUG:' "${1}"
	fi
}

rebuild_trash_dir () {
	debug "rebuilding trash directory ${TEST_TRASH_DIR}"

	# clear/create trash directory
	if [[ -d "$TEST_TRASH_DIR" ]]; then
		rm -rf "${TEST_TRASH_DIR}"
	fi

	mkdir "${TEST_TRASH_DIR}"
}

# Test Setup
#
#
if [[ -n "${TEST_DEBUG+x}" ]]; then
	set -x
fi

# Verify that git-chat binaries exist
if [[ -n "${TEST_GIT_CHAT_INSTALLED+x}" ]]; then
	if ! type "${TEST_GIT_CHAT_INSTALLED}/git-chat" >/dev/null 2>&1; then
		echo "Could not find git-chat binaries in the provided installation location:" 1>&2
		echo "${TEST_GIT_CHAT_INSTALLED}" 1>&2
		exit 1
	fi
elif ! type git-chat >/dev/null 2>&1; then
	echo "Could not find git-chat binaries on the path:" 1>&2
	echo "${PATH}" 1>&2
	exit 1
fi

if [[ ! -d "${TEST_RUNNER_PATH}/integration" ]]; then
	echo "No tests were found; directory does not exist: ${TEST_RUNNER_PATH}/integration" 1>&2
	exit 1
fi

export PATH="${TEST_RUNNER_PATH}/integration/bin:${PATH}"
if [[ -n "${TEST_VALGRIND+x}" ]]; then
	# When running integration tests under valgrind memcheck, we wrap the git-chat
	# executable in valgrind.sh, which accepts the same arguments as git-chat.
	# Then, we symlink git-chat to valgrind.sh, and put it on the PATH.

	if [[ ! -d "${TEST_RUNNER_PATH}/integration/valgrind" ]]; then
		echo "Valgrind tooling does not exist: ${TEST_RUNNER_PATH}/integration/valgrind" 1>&2
		exit 1
	fi

	if [[ -n "${TEST_GIT_CHAT_INSTALLED+x}" ]]; then
		export VALGRIND_TARGET="${TEST_GIT_CHAT_INSTALLED}/git-chat"
	else
		export VALGRIND_TARGET="$(which git-chat)"
	fi

	rm -f "${TEST_RUNNER_PATH}/integration/valgrind/git-chat"
	ln -s "${TEST_RUNNER_PATH}/integration/valgrind/valgrind.sh" "${TEST_RUNNER_PATH}/integration/valgrind/git-chat"
elif [[ -n "${TEST_GIT_CHAT_INSTALLED+x}" ]]; then
	export PATH="${TEST_GIT_CHAT_INSTALLED}:${PATH}"
fi

if ! type gpg2 >/dev/null 2>&1; then
	# In certain environments, gpg2 does not exist. In that case create a symlink
	# in the integration test bin/ directory to the gpg executable

	if ! type gpg >/dev/null 2>&1; then
		echo "Could not find gpg or gpg2 binaries; cannot execute test suite" 1>&2
		exit 1
	fi

	rm -f "${TEST_RUNNER_PATH}/integration/bin/gpg2"
	ln -s "$(which gpg)" "${TEST_RUNNER_PATH}/integration/bin/gpg2"
fi

debug "PATH: ${PATH}"

if [[ -z "${TEST_TRASH_DIR+x}" ]]; then
	TEST_TRASH_DIR="${TEST_RUNNER_PATH}/trash"
fi

if [ ! -d $TEST_RUNNER_PATH/integration ]; then
	echo "No tests were found; directory does not exist: ${TEST_RUNNER_PATH}/integration" 1>&2
fi

TEST_RESOURCES_DIR="${TEST_RUNNER_PATH}/resources"
debug "resources directory path: ${TEST_RESOURCES_DIR}"

# Clear any existing logs
rm -f "${TEST_RUNNER_PATH}/out.log"

debug "running tests from path: $TEST_RUNNER_PATH/integration"

export TEST_TRASH_DIR
export TEST_VERBOSE
export TEST_VALGRIND
export TEST_DEBUG
export TEST_NO_COLOR_OUT
export TEST_RESOURCES_DIR
export TEST_RUNNER_PATH

# Trick Git into thinking that the test trash directory is not in a git working tree
export GIT_CEILING_DIRECTORIES="$(dirname "$TEST_TRASH_DIR")"

# Test Execution
#
#
cd "${TEST_RUNNER_PATH}/integration" || exit 1

TESTS="${TEST_RUNNER_PATH}/integration/${TEST_PATTERN}"
TEST_FAILURES=0
for test_path in $TESTS; do
	rebuild_trash_dir

	if [[ ! -x "$test_path" ]]; then
		echo "Integration test file is not executable: $test_path" 1>&2
		exit 1
	fi

	echo '***' "$(basename -- "${test_path}")" '***' >>"${TEST_RUNNER_PATH}/out.log"

	if ! bash "${test_path}"; then
		TEST_FAILURES=$((TEST_FAILURES + 1))
	fi

	echo
done

debug "finished running tests"


# Print Test Summary
#
#
echo "Test execution completed with ${TEST_FAILURES} test failures."
if [[ ${TEST_FAILURES} -ne 0 ]]; then
	echo "Run with --debug for more details."
	exit 1
fi

exit 0
