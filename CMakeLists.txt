cmake_policy(SET CMP0048 NEW)
project(git-chat VERSION 0.0.1 DESCRIPTION "A Git-Based Command-Line Messaging Application" LANGUAGES C)

cmake_minimum_required(VERSION 3.7.2)
set(CMAKE_C_STANDARD 11)

set(CMAKE_MODULE_PATH ${CMAKE_MODULE_PATH} "${CMAKE_SOURCE_DIR}/cmake/Modules/")

if (CMAKE_INSTALL_PREFIX_INITIALIZED_TO_DEFAULT)
	set(CMAKE_INSTALL_PREFIX $ENV{HOME}/ CACHE PATH "Install prefix default" FORCE)
endif (CMAKE_INSTALL_PREFIX_INITIALIZED_TO_DEFAULT)

if (CMAKE_C_COMPILER_ID STREQUAL "GNU")
	add_definitions(-Wall -Werror -pedantic)
elseif (CMAKE_C_COMPILER_ID STREQUAL "Clang")
	add_definitions(-Wall -Wextra -pedantic)
endif ()


#
# Configure Project
#
execute_process(
		COMMAND git rev-parse --short HEAD
		WORKING_DIRECTORY ${CMAKE_SOURCE_DIR}
		OUTPUT_VARIABLE GIT_CHAT_GIT_REV
		OUTPUT_STRIP_TRAILING_WHITESPACE
		RESULT_VARIABLE git_rev_parse_ret
)

if (NOT git_rev_parse_ret EQUAL "0")
	set(GIT_CHAT_GIT_REV "unknown-rev")
endif ()

if (NOT CMAKE_BUILD_TYPE MATCHES RELEASE)
	set(GIT_CHAT_VERSION "${CMAKE_PROJECT_VERSION}.git-${GIT_CHAT_GIT_REV}")
else ()
	set(GIT_CHAT_VERSION ${CMAKE_PROJECT_VERSION})
endif (NOT CMAKE_BUILD_TYPE MATCHES RELEASE)

option(RUNTIME_LOGGING "Compile with logging enabled" ON)
if (RUNTIME_LOGGING)
	add_definitions(-DRUNTIME_LOGGING)
endif (RUNTIME_LOGGING)

configure_file(
	"${PROJECT_SOURCE_DIR}/templates/version.h.in"
	"${PROJECT_BINARY_DIR}/include/version.h"
)


#
# Configure git-chat Executable and Installation
#
find_package(GPGME REQUIRED)
if (NOT GPGME_VANILLA_FOUND)
	message(FATAL_ERROR "Failed to locate GPGME dependency (GPGME_VANILLA)")
endif ()

file(GLOB_RECURSE src_list ${CMAKE_CURRENT_SOURCE_DIR}/src/*.c)
list(REMOVE_ITEM src_list ${CMAKE_CURRENT_SOURCE_DIR}/src/git-chat.c)

# build static library so that tests don't have to compile sources twice
add_library(git-chat-internal STATIC ${src_list})
target_link_libraries(git-chat-internal m ${GPGME_VANILLA_LIBRARIES})
target_include_directories(git-chat-internal PRIVATE
		"${CMAKE_CURRENT_SOURCE_DIR}/include/"
		"${CMAKE_CURRENT_BINARY_DIR}/include/"
		"${GPGME_INCLUDES}"
)

add_executable(git-chat ${CMAKE_CURRENT_SOURCE_DIR}/src/git-chat.c)
target_link_libraries(git-chat git-chat-internal)
target_include_directories(git-chat PRIVATE
		"${CMAKE_CURRENT_SOURCE_DIR}/include/"
		"${CMAKE_CURRENT_BINARY_DIR}/include/"
		"${GPGME_INCLUDES}"
)

install(TARGETS git-chat RUNTIME DESTINATION bin)
install(DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR}/share/ DESTINATION share/git-chat)

#
# Configure Man Page Installation
#
add_subdirectory(${CMAKE_CURRENT_SOURCE_DIR}/docs)


#
# Configure Unit and Integration Tests
#
enable_testing()
add_subdirectory(${CMAKE_CURRENT_SOURCE_DIR}/test)

#
# Package RPM/DEB
#
include(cmake/packaging.cmake)
include(CPack)