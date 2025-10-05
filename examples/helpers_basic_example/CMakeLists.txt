# ----------------------------------------------------------------------
# Copyright 2025 Jody Hagins
# Distributed under the MIT Software License
# See accompanying file LICENSE or copy at
# https://opensource.org/licenses/MIT
# ----------------------------------------------------------------------
cmake_minimum_required(VERSION 3.20)

project(AtlasHelpersBasicExample
    LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

include(FetchContent)

# Fetch Atlas from the parent directory (in real usage, this would be a git URL)
fetchcontent_declare(Atlas
    SOURCE_DIR ${CMAKE_CURRENT_SOURCE_DIR}/../..)

fetchcontent_makeavailable(Atlas)

# Create our library with some types
add_library(example_types INTERFACE)

# Method 1: Simplified single-type function
# Note: Explicitly specify NAMESPACE since this example doesn't follow src/ structure
atlas_add_type(UserId int "==, !=, <=>"
    NAMESPACE example
    TARGET example_types)

# Method 2: Full function with all options specified
add_atlas_strong_type(
    NAME Price
    TYPE double
    DESCRIPTION "+, -, <=>, out"
    NAMESPACE example
    DEFAULT_VALUE 0.0
    TARGET example_types)

# Method 3: Generate without automatic target integration
# Useful when you want more control over source management
add_atlas_strong_type(
    NAME Handle
    TYPE size_t
    DESCRIPTION "->, ==, !=, bool"
    NAMESPACE example
    OUTPUT ${CMAKE_CURRENT_SOURCE_DIR}/Handle.hpp
    # No TARGET specified - just generates the file
)

# Manually add the Handle.hpp to our target
target_sources(example_types
    INTERFACE
        ${CMAKE_CURRENT_SOURCE_DIR}/Handle.hpp)

# Main executable
add_executable(basic_example main.cpp)

target_link_libraries(basic_example
    PRIVATE
        example_types)

target_include_directories(basic_example
    PRIVATE
        ${CMAKE_CURRENT_SOURCE_DIR})
