cmake_minimum_required(VERSION 2.6)

set(PROJECT_NAME git-chat)
project(${PROJECT_NAME})

# version
set(CMAKE_PROJECT_VERSION_MAJOR 0)
set(CMAKE_PROJECT_VERSION_MINOR 1)

configure_file (
    "${PROJECT_SOURCE_DIR}/version.h.in"
    "${PROJECT_BINARY_DIR}/version.h"
)

set(CMAKE_C_STANDARD 99)

# compiler options
if(CMAKE_COMPILER_IS_GNUCC)
    add_definitions(-Wall -Werror -pedantic -std=c99)
endif(CMAKE_COMPILER_IS_GNUCC)

include_directories(${PROJECT_BINARY_DIR})

# add library sources
file(GLOB_RECURSE LIB_SRC_FILES *.c)
add_library(${PROJECT_NAME}_LIB STATIC ${LIB_SRC_FILES})

# add executable
set(SOURCE_FILES main.c)
add_executable(${PROJECT_NAME} ${SOURCE_FILES})
target_link_libraries(${PROJECT_NAME} ${PROJECT_NAME}_LIB)
install(TARGETS ${PROJECT_NAME} DESTINATION bin)
