# ----------------------------------------------------------------------
# Copyright 2025 Jody Hagins
# Distributed under the MIT Software License
# See accompanying file LICENSE or copy at
# https://opensource.org/licenses/MIT
# ----------------------------------------------------------------------
cmake_minimum_required(VERSION 3.20)

project(AtlasHelpersInlineExample
    LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

include(FetchContent)

# Fetch Atlas from the parent directory (in real usage, this would be a git URL)
fetchcontent_declare(Atlas
    SOURCE_DIR ${CMAKE_CURRENT_SOURCE_DIR}/../..)

fetchcontent_makeavailable(Atlas)

# Create our library
add_library(inline_types INTERFACE)

# Define multiple types inline without needing a separate file
# This is great for keeping type definitions close to where they're used
add_atlas_strong_types_inline(
    OUTPUT ${CMAKE_CURRENT_BINARY_DIR}/CommonTypes.hpp
    TARGET inline_types
    CONTENT
    "
# Common strong types for our application
guard_prefix=COMMON_TYPES

[type]
kind=struct
namespace=app
name=UserId
description=strong int; ==, !=, no-constexpr
default_value=-1

[type]
kind=struct
namespace=app
name=SessionId
description=strong int; ==, !=, no-constexpr
default_value=-1

[type]
kind=struct
namespace=app
name=Distance
description=strong double; +, -, *, /, ==, !=, <, <=, >, >=, out, no-constexpr
default_value=0.0

[type]
kind=struct
namespace=app
name=Timestamp
description=strong int64_t; ==, !=, <, <=, >, >=, out, no-constexpr
")

# Main executable
add_executable(inline_example main.cpp)

target_link_libraries(inline_example
    PRIVATE
        inline_types)

target_include_directories(inline_example
    PRIVATE
        ${CMAKE_CURRENT_BINARY_DIR})
