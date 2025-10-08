# ----------------------------------------------------------------------
# Copyright 2025 Jody Hagins
# Distributed under the MIT Software License
# See accompanying file LICENSE or copy at
# https://opensource.org/licenses/MIT
# ----------------------------------------------------------------------
cmake_minimum_required(VERSION 3.20)
project(AtlasFetchContentExample LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

include(FetchContent)

# Fetch Atlas from the parent directory (in real usage, this would be a git URL)
fetchcontent_declare(Atlas SOURCE_DIR ${CMAKE_CURRENT_SOURCE_DIR}/../..)

# Let Atlas use FetchContent for Boost (default behavior)
# This ensures the example works even when Boost is not installed on the system
fetchcontent_makeavailable(Atlas)

# Note: This example now uses Atlas CMake helper functions!
# The helpers are automatically available after FetchContent_MakeAvailable(Atlas)

# Create a library for generated types
add_library(example_types INTERFACE)

# Generate strong types using the helper functions
# Much simpler than manual add_custom_command!

# Method 1: Using the simplified atlas_add_type function
# Note: Must specify NAMESPACE since examples don't follow src/ structure
atlas_add_type(UserId int "==, !=, no-constexpr"
    NAMESPACE example
    TARGET example_types
    OUTPUT ${CMAKE_CURRENT_BINARY_DIR}/UserId.hpp)

# Method 2: Using the full add_atlas_strong_type function
add_atlas_strong_type(
    NAME Price
    TYPE double
    DESCRIPTION "+, -, ==, !=, <, <=, >, >=, out, no-constexpr"
    NAMESPACE example
    TARGET example_types
    OUTPUT ${CMAKE_CURRENT_BINARY_DIR}/Price.hpp)

# Main executable
add_executable(example_app main.cpp)

target_link_libraries(example_app
    PRIVATE
        example_types)

target_include_directories(example_app
    PRIVATE
        ${CMAKE_CURRENT_BINARY_DIR})
