# Initialize Build Environment
cmake_minimum_required(VERSION 3.18.4)

set(CMAKE_C_STANDARD 11)

project(git-chat
	DESCRIPTION "A Git-Based Command-Line Messaging Application"
	HOMEPAGE_URL "https://github.com/brandon1024/gitchat"
	LANGUAGES C
	VERSION 0.0.1
)

include(GNUInstallDirs)

set(CMAKE_C_FLAGS "-Wall -Werror -pedantic")
set(CMAKE_C_FLAGS_DEBUG "-g -ggdb")

list(APPEND CMAKE_MODULE_PATH "${CMAKE_CURRENT_SOURCE_DIR}/cmake/Modules/")

# Configure Build Options
option(RUNTIME_LOGGING "Compile with logging enabled" ON)

# Read Version Information
execute_process(
		COMMAND git rev-parse --short HEAD
		WORKING_DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR}
		OUTPUT_VARIABLE GIT_CHAT_GIT_REV
		OUTPUT_STRIP_TRAILING_WHITESPACE
		RESULT_VARIABLE git_rev_parse_ret
)

if (NOT git_rev_parse_ret EQUAL "0")
	set(GIT_CHAT_GIT_REV "unknown-rev")
endif ()

set(GIT_CHAT_VERSION "${CMAKE_PROJECT_VERSION}.git-${GIT_CHAT_GIT_REV}")

configure_file(
	"${CMAKE_CURRENT_SOURCE_DIR}/templates/version.h.in"
	"${CMAKE_CURRENT_BINARY_DIR}/include/version.h"
)

# Load Project Dependencies
find_package(GPGME REQUIRED)

# Configure Build Targets
set(GITCHAT_BUILD_DEFINITIONS
	$<$<BOOL:${RUNTIME_LOGGING}>:RUNTIME_LOGGING>
	$<$<CONFIG:Debug>:DEBUGGING_ENABLED>
)

file(GLOB_RECURSE src_list ${CMAKE_CURRENT_SOURCE_DIR}/src/*.c)
list(REMOVE_ITEM src_list ${CMAKE_CURRENT_SOURCE_DIR}/src/git-chat.c)

# build static library so that tests don't have to compile sources twice
add_library(git-chat-internal STATIC ${src_list})
target_compile_definitions(git-chat-internal PUBLIC ${GITCHAT_BUILD_DEFINITIONS})
target_link_libraries(git-chat-internal m GPGME::libgpgme)
target_include_directories(git-chat-internal PRIVATE
		"${CMAKE_CURRENT_SOURCE_DIR}/include/"
		"${CMAKE_CURRENT_BINARY_DIR}/include/"
)

add_executable(git-chat ${CMAKE_CURRENT_SOURCE_DIR}/src/git-chat.c)
target_compile_definitions(git-chat-internal PUBLIC ${GITCHAT_BUILD_DEFINITIONS})
target_link_libraries(git-chat git-chat-internal)
target_include_directories(git-chat PRIVATE
		"${CMAKE_CURRENT_SOURCE_DIR}/include/"
		"${CMAKE_CURRENT_BINARY_DIR}/include/"
)

# Configure Install Targets
if(CMAKE_INSTALL_PREFIX_INITIALIZED_TO_DEFAULT)
  set_property(CACHE CMAKE_INSTALL_PREFIX PROPERTY VALUE "$ENV{HOME}/")
endif()

install(TARGETS git-chat RUNTIME DESTINATION ${CMAKE_INSTALL_BINDIR})
install(DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR}/share/ DESTINATION ${CMAKE_INSTALL_DATAROOTDIR}/git-chat)

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
