# Integration Test Library
#
# All test library functions and setup should exist here, and must be sourced by
# each integration test.

ASSERT_PASSED=0
ASSERT_FAILED=1
COLOR_RED='\033[0;31m'
COLOR_GREEN='\033[0;32m'
COLOR_RESET='\033[0m'

ASSERT_TEST_NUMBER=1

if [ ! -z "${TEST_DEBUG}" ]; then
	export GIT_CHAT_LOG_LEVEL=TRACE
	set -x
fi

if [ ! -z "${TEST_NO_COLOR_OUT}" ]; then
	COLOR_RED=''
	COLOR_GREEN=''
	COLOR_RESET=''
fi

if [ -z "${TEST_TRASH_DIR}" ]; then
	echo "TEST_TRASH_DIR is not defined. Cannot run tests." 1>&2
	exit 1
fi

cd $TEST_TRASH_DIR

fail_test () {
	printf "${COLOR_RED}not ok ${ASSERT_TEST_NUMBER} - ${1}${COLOR_RESET}\n" 1>&2
}

pass_test () {
	printf "${COLOR_GREEN}ok ${ASSERT_TEST_NUMBER} - ${1}${COLOR_RESET}\n"
}

# Evaluate a command or script and assert that it can execute and return with a
# zero exit status.
# usage: assert_success <message> [<test setup>] <script>
#
assert_success () {
	if [[ ${#} -eq 2 ]]; then
		if [ ! -z "${TEST_DEBUG}" ] || [ ! -z "${TEST_VERBOSE}" ]; then
			(eval "$2")
		else
			(eval "$2") > /dev/null 2>&1
		fi
	elif [[ ${#} -eq 3 ]]; then
		if [ ! -z "${TEST_DEBUG}" ] || [ ! -z "${TEST_VERBOSE}" ]; then
			(eval "$2")
			(eval "$3")
		else
			(eval "$2") > /dev/null 2>&1
			(eval "$3") > /dev/null 2>&1
		fi
	else
		echo 'unexpected number of arguments passed to assert_success' 1>&2
		exit 1
	fi

	if [[ ${?} -eq 0 ]]; then
		pass_test "$1"
		ASSERT_TEST_NUMBER=$((ASSERT_TEST_NUMBER + 1))

		return $ASSERT_PASSED
	fi

	fail_test "$1"
	exit $ASSERT_FAILED
}

# Evaluate a command or script and assert that it fails with a non-zero exit status.
# usage: assert_failure <message> [<test setup>] <script>
#
assert_failure () {
	if [[ ${#} -eq 2 ]]; then
		if [ ! -z "${TEST_DEBUG}" ] || [ ! -z "${TEST_VERBOSE}" ]; then
			(eval "$2")
		else
			(eval "$2") > /dev/null 2>&1
		fi
	elif [[ ${#} -eq 3 ]]; then
		if [ ! -z "${TEST_DEBUG}" ] || [ ! -z "${TEST_VERBOSE}" ]; then
			(eval "$2")
			(eval "$3")
		else
			(eval "$2") > /dev/null 2>&1
			(eval "$3") > /dev/null 2>&1
		fi
	else
		echo 'unexpected number of arguments passed to assert_failure' 1>&2
		exit 1
	fi

	if [[ ${?} -ne 0 ]]; then
		pass_test "$1"
		ASSERT_TEST_NUMBER=$((ASSERT_TEST_NUMBER + 1))

		return $ASSERT_PASSED
	fi

	fail_test "$1"
	exit $ASSERT_FAILED
}

reset_trash_dir () {
	rm -rf ..?* .[!.]* *
	return 0
}