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
FetchContent_Declare(
    Atlas
    SOURCE_DIR ${CMAKE_CURRENT_SOURCE_DIR}/../..
)

# Let Atlas use FetchContent for Boost (default behavior)
# This ensures the example works even when Boost is not installed on the system
FetchContent_MakeAvailable(Atlas)

# Directory for generated headers
set(GENERATED_DIR "${CMAKE_CURRENT_BINARY_DIR}/generated")
file(MAKE_DIRECTORY ${GENERATED_DIR})

# Generate a UserId strong type
set(USERID_HEADER "${GENERATED_DIR}/UserId.hpp")
set(USERID_DESC "strong int\\; ==, !=, <=>")
add_custom_command(
    OUTPUT ${USERID_HEADER}
    COMMAND ${Atlas_EXECUTABLE} --kind=struct --namespace=example --name=UserId --description=${USERID_DESC} > ${USERID_HEADER}
    DEPENDS Atlas::atlas
    COMMENT "Generating UserId strong type"
    VERBATIM
)

# Generate a Price strong type
set(PRICE_HEADER "${GENERATED_DIR}/Price.hpp")
set(PRICE_DESC "strong double\\; +, -, *, /, ==, !=, <=>, out")
add_custom_command(
    OUTPUT ${PRICE_HEADER}
    COMMAND ${Atlas_EXECUTABLE} --kind=struct --namespace=example --name=Price --description=${PRICE_DESC} > ${PRICE_HEADER}
    DEPENDS Atlas::atlas
    COMMENT "Generating Price strong type"
    VERBATIM
)

# Custom target that depends on generated files
add_custom_target(generate_types
    DEPENDS ${USERID_HEADER} ${PRICE_HEADER}
)

# Main executable
add_executable(example_app main.cpp)
add_dependencies(example_app generate_types)
target_include_directories(example_app PRIVATE ${GENERATED_DIR})