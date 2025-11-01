# ----------------------------------------------------------------------
# Copyright 2025 Jody Hagins
# Distributed under the MIT Software License
# See accompanying file LICENSE or copy at
# https://opensource.org/licenses/MIT
# ----------------------------------------------------------------------

# OperatorParser unit tests
add_executable(OperatorParser_ut OperatorParser_ut.cpp)

target_link_libraries(OperatorParser_ut
    PRIVATE
        generation
        doctest::doctest)

target_include_directories(OperatorParser_ut
    PRIVATE
        ${CMAKE_SOURCE_DIR}/src/lib
        ${CMAKE_SOURCE_DIR})

target_apply_atlas_warnings(OperatorParser_ut)

# Register with CTest
add_test(NAME OperatorParser_ut COMMAND OperatorParser_ut)
