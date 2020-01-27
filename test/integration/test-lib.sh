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

if [[ -n "${TEST_DEBUG+x}" ]]; then
	set -x
fi

if [[ -n "${TEST_NO_COLOR_OUT+x}" ]]; then
	COLOR_RED=''
	COLOR_GREEN=''
	COLOR_RESET=''
fi

if [[ -z "${TEST_TRASH_DIR+x}" ]]; then
	echo "TEST_TRASH_DIR is not defined. Cannot run tests." 1>&2
	exit 1
fi

cd "${TEST_TRASH_DIR}" || exit 1

fail_test () {
	printf "  ${COLOR_RED}not ok $ASSERT_TEST_NUMBER - ${1}${COLOR_RESET}\n" 1>&2
}

pass_test () {
	printf "  ${COLOR_GREEN}ok $ASSERT_TEST_NUMBER - ${1}${COLOR_RESET}\n" 1>&2
}

# Evaluate a command or script and assert that it can execute and return with a
# zero exit status.
# usage: assert_success <message> [<test setup>] <script>
#
assert_success () {
	if [[ ${#} -eq 2 ]]; then
		if [[ -n "${TEST_DEBUG+x}" || -n "${TEST_VERBOSE+x}" ]]; then
			(eval "$2")
		else
			(eval "$2") >>"${TEST_RUNNER_PATH}/out.log" 2>&1
		fi
	elif [[ ${#} -eq 3 ]]; then
		if [[ -n "${TEST_DEBUG+x}" || -n "${TEST_VERBOSE+x}" ]]; then
			(
				eval "${2}"
				eval "${3}"
			)
		else
			(
				eval "${2}" >>"${TEST_RUNNER_PATH}/out.log" 2>&1
				eval "${3}" >>"${TEST_RUNNER_PATH}/out.log" 2>&1
			)
		fi
	else
		echo 'unexpected number of arguments passed to assert_success' 1>&2
		exit 1
	fi

	test_status="${?}"
	if [[ ${test_status} -eq 0 ]]; then
		pass_test "${1}" 2>&1 | tee -a "${TEST_RUNNER_PATH}/out.log"
		ASSERT_TEST_NUMBER=$((ASSERT_TEST_NUMBER + 1))

		return ${ASSERT_PASSED}
	fi

	fail_test "${1}" 2>&1 | tee -a "${TEST_RUNNER_PATH}/out.log"
	exit ${ASSERT_FAILED}
}

# Evaluate a command or script and assert that it fails with a non-zero exit status.
# usage: assert_failure <message> [<test setup>] <script>
#
assert_failure () {
	if [[ ${#} -eq 2 ]]; then
		if [[ -n "${TEST_DEBUG+x}" || -n "${TEST_VERBOSE+x}" ]]; then
			(eval "${2}")
		else
			(eval "${2}") >>"${TEST_RUNNER_PATH}/out.log" 2>&1
		fi
	elif [[ ${#} -eq 3 ]]; then
		if [[ -n "${TEST_DEBUG+x}" || -n "${TEST_VERBOSE+x}" ]]; then
			(
				eval "${2}"
				eval "${3}"
			)
		else
			(
				eval "${2}" >>"${TEST_RUNNER_PATH}/out.log" 2>&1
				eval "${3}" >>"${TEST_RUNNER_PATH}/out.log" 2>&1
			)
		fi
	else
		echo 'unexpected number of arguments passed to assert_failure' 1>&2
		exit 1
	fi

	test_status="${?}"
	if [[ ${test_status} -ne 0 ]]; then
		pass_test "${1}" 2>&1 | tee -a "${TEST_RUNNER_PATH}/out.log"
		ASSERT_TEST_NUMBER=$((ASSERT_TEST_NUMBER + 1))

		return ${ASSERT_PASSED}
	fi

	fail_test "${1}" 2>&1 | tee -a "${TEST_RUNNER_PATH}/out.log"
	exit ${ASSERT_FAILED}
}

reset_trash_dir () {
	rm -rf -- ..?* .[!.]* *
	return 0
}

setup_test_gpg () {
	rm -rf "${TEST_TRASH_DIR}/.gpg_tmp"
	mkdir -m 700 "${TEST_TRASH_DIR}/.gpg_tmp"
	export GNUPGHOME="${TEST_TRASH_DIR}/.gpg_tmp"
	unset GPG_AGENT_INFO

	private_key="${TEST_RESOURCES_DIR}/gpgkeys/test_user.gpg"
	if [[ -n "${1+x}" ]]; then
		private_key="${1}"
	fi

	gpg2 --batch --allow-secret-key-import --import "${private_key}"
}
