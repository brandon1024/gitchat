#!/bin/sh

set -e

# Install git-chat
sudo cmake --build . --target install
which git-chat

# Run unit tests
ctest --verbose

# Run valgrind, if Linux
if [ "$TRAVIS_OS_NAME" = "linux" ]; then
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