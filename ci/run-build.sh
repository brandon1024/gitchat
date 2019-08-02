#!/usr/bin/env bash
#
# Travis CI build script for installing git-chat and executing test suites.

set -e

# Install git-chat
sudo make all install
which git-chat

# Run tests
ctest --verbose

# Run valgrind, if Linux
if [ "${TRAVIS_OS_NAME}" = "linux" ]; then
    cd test
    valgrind \
        --tool=memcheck \
        --gen-suppressions=all \
        --leak-check=full \
        --leak-resolution=high \
        --track-origins=yes \
        --vgdb=no \
        --error-exitcode=1 \
        ./git-chat-unit-tests
fi
