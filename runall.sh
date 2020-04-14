#!/bin/bash

TEST_DIR="${0%/*}"

BUILD_DIR="${TEST_DIR}/../cmake-build-debug"
TEST_EXEC="${BUILD_DIR}/tests/theTests"

mkdir -p ${BUILD_DIR}
cd ${BUILD_DIR}
cmake ..
make

TESTS=`${TEST_EXEC} --gtest_list_tests | grep '^Test'`
for TEST in $TESTS
do
  (${TEST_EXEC} --gtest_filter="${TEST}*")
done

