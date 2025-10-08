# ----------------------------------------------------------------------
# Copyright 2025 Jody Hagins
# Distributed under the MIT Software License
# See accompanying file LICENSE or copy at
# https://opensource.org/licenses/MIT
# ----------------------------------------------------------------------
cmake_minimum_required(VERSION 3.20)

project(AtlasHelpersFileExample LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

include(FetchContent)

# Fetch Atlas from the parent directory (in real usage, this would be a git URL)
fetchcontent_declare(Atlas
    SOURCE_DIR ${CMAKE_CURRENT_SOURCE_DIR}/../..)

fetchcontent_makeavailable(Atlas)

# Create our library
add_library(file_types INTERFACE)

# Generate types from an external configuration file
# This approach is best for larger projects with many types,
# or when you want to share type definitions across projects
add_atlas_strong_types_from_file(
    INPUT ${CMAKE_CURRENT_SOURCE_DIR}/types.txt
    OUTPUT ${CMAKE_CURRENT_BINARY_DIR}/DomainTypes.hpp
    TARGET file_types)

# Main executable
add_executable(file_example main.cpp)

target_link_libraries(file_example
    PRIVATE
        file_types)

target_include_directories(file_example
    PRIVATE
        ${CMAKE_CURRENT_BINARY_DIR})
