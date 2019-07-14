#!/bin/sh
#
# Integration test runner for all integration tests.
#

set -e

rm -rf int-tmp
mkdir int-tmp

TESTS="$(pwd)/integration/t*.sh"

# for each integration test
cd integration
for test in $TESTS; do
    sh $test
done