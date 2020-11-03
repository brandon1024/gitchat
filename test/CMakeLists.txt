#
# Prepare Unit Tests
#
add_custom_target(copy-test-resources ALL
		COMMAND ${CMAKE_COMMAND} -E copy_directory ${CMAKE_CURRENT_SOURCE_DIR}/resources ${CMAKE_CURRENT_BINARY_DIR}/resources)

add_library(unit-test-common runner.c test-lib.c)
add_dependencies(unit-test-common copy-test-resources)
target_link_libraries(unit-test-common git-chat-internal)
target_include_directories(unit-test-common PRIVATE
		"${PROJECT_SOURCE_DIR}/include/"
		"${PROJECT_BINARY_DIR}/include/"
		"${CMAKE_CURRENT_SOURCE_DIR}/include/")

function(add_unit_test test_name test_sources)
	add_executable(${test_name} ${test_sources})
	target_link_libraries(${test_name} unit-test-common)
	target_compile_definitions(${test_name} PRIVATE SUITE_NAME="${test_name}")
	target_include_directories(${test_name} PRIVATE
			"${PROJECT_SOURCE_DIR}/include/"
			"${PROJECT_BINARY_DIR}/include/"
			"${CMAKE_CURRENT_SOURCE_DIR}/include/")

	add_test(NAME ${test_name} COMMAND ${test_name})
endfunction()

set(MEMORYCHECK_COMMAND_OPTIONS "--gen-suppressions=all --leak-check=full --leak-resolution=high --track-origins=yes --vgdb=no --error-exitcode=1")
include(CTest)

#
# Add Unit Tests
#
add_unit_test(argv-array-test ${CMAKE_CURRENT_SOURCE_DIR}/unit/argv-array-test.c)
add_unit_test(config-data-test ${CMAKE_CURRENT_SOURCE_DIR}/unit/config-data-test.c)
add_unit_test(config-defaults-test ${CMAKE_CURRENT_SOURCE_DIR}/unit/config-defaults-test.c)
add_unit_test(config-key-test ${CMAKE_CURRENT_SOURCE_DIR}/unit/config-key-test.c)
add_unit_test(fs-utils-test ${CMAKE_CURRENT_SOURCE_DIR}/unit/fs-utils-test.c)
add_unit_test(git-commit-parse-test ${CMAKE_CURRENT_SOURCE_DIR}/unit/git-commit-parse-test.c)
add_unit_test(node-visitor-test ${CMAKE_CURRENT_SOURCE_DIR}/unit/node-visitor-test.c)
add_unit_test(parse-config-test ${CMAKE_CURRENT_SOURCE_DIR}/unit/parse-config-test.c)
add_unit_test(parse-options-test ${CMAKE_CURRENT_SOURCE_DIR}/unit/parse-options-test.c)
add_unit_test(run-command-test ${CMAKE_CURRENT_SOURCE_DIR}/unit/run-command-test.c)
add_unit_test(str-array-test ${CMAKE_CURRENT_SOURCE_DIR}/unit/str-array-test.c)
add_unit_test(strbuf-test ${CMAKE_CURRENT_SOURCE_DIR}/unit/strbuf-test.c)

#
# Prepare Integration Tests
#
add_custom_target(copy-integration-tests ALL
		COMMAND ${CMAKE_COMMAND} -E copy_directory ${CMAKE_CURRENT_SOURCE_DIR}/integration ${CMAKE_CURRENT_BINARY_DIR}/integration)
add_custom_target(integration
		COMMAND ${BASH_INTERPRETER} ${CMAKE_CURRENT_SOURCE_DIR}/integration-runner.sh
		DEPENDS copy-integration-tests copy-test-resources
		WORKING_DIRECTORY ${CMAKE_CURRENT_BINARY_DIR})