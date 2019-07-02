#!/bin/sh

# Install git-chat
sudo cmake --build . --target install
which git-chat

# Run unit tests
./git-chat-test

# Run valgrind, if Linux
if [ "$TRAVIS_OS_NAME" = "linux" ]; then
    valgrind \
        --tool=memcheck \
        --gen-suppressions=all \
        --leak-check=full \
        --leak-resolution=high \
        --track-origins=yes \
        --vgdb=no \
        --error-exitcode=1 \
        ./git-chat-test
fi