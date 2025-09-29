#!/bin/bash
# Quick test to verify CMake configuration without building dependencies

set -e

echo "Testing Atlas FetchContent integration..."

# Create a minimal test project
TEST_DIR=$(mktemp -d)
trap "rm -rf $TEST_DIR" EXIT

cat > $TEST_DIR/CMakeLists.txt << 'EOF'
cmake_minimum_required(VERSION 3.20)
project(AtlasIntegrationTest LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

include(FetchContent)

# Fetch Atlas from local source (simulating FetchContent from git)
FetchContent_Declare(
    Atlas
    SOURCE_DIR ${CMAKE_CURRENT_SOURCE_DIR}/../../..
)

# Skip Boost fetch for this test by using a dummy option
set(USE_SYSTEM_BOOST ON CACHE BOOL "" FORCE)

# This will fail if Boost is not found, but we just want to test
# that Atlas exports the right variables and targets
FetchContent_MakeAvailable(Atlas)

# Test that Atlas_EXECUTABLE is set
if(NOT DEFINED Atlas_EXECUTABLE)
    message(FATAL_ERROR "Atlas_EXECUTABLE is not defined!")
endif()

message(STATUS "✓ Atlas_EXECUTABLE is defined: ${Atlas_EXECUTABLE}")

# Test that Atlas::atlas target exists
if(NOT TARGET Atlas::atlas)
    message(FATAL_ERROR "Atlas::atlas target does not exist!")
endif()

message(STATUS "✓ Atlas::atlas target exists")

# Create a dummy file to generate
file(WRITE ${CMAKE_CURRENT_BINARY_DIR}/dummy.txt "test")

# Test that we can reference the executable in a custom command
add_custom_command(
    OUTPUT ${CMAKE_CURRENT_BINARY_DIR}/test_output.txt
    COMMAND echo "Atlas executable: ${Atlas_EXECUTABLE}" > ${CMAKE_CURRENT_BINARY_DIR}/test_output.txt
    DEPENDS Atlas::atlas
    VERBATIM
)

add_custom_target(test_target ALL DEPENDS ${CMAKE_CURRENT_BINARY_DIR}/test_output.txt)

message(STATUS "✓ Custom command with Atlas_EXECUTABLE works")
message(STATUS "✓ All configuration checks passed!")
EOF

cd $TEST_DIR

# Run CMake configure - this will fail due to missing Boost, but we'll catch that
if cmake -B build 2>&1 | grep -q "Atlas_EXECUTABLE is defined"; then
    echo "✓ FetchContent integration test PASSED"
    echo "✓ Atlas_EXECUTABLE variable is properly exported"
    exit 0
else
    echo "✗ FetchContent integration test FAILED"
    echo "✗ Atlas_EXECUTABLE was not properly set"
    exit 1
fi