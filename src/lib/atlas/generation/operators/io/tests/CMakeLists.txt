# ----------------------------------------------------------------------
# Copyright 2025 Jody Hagins
# Distributed under the MIT Software License
# See accompanying file LICENSE or copy at
# https://opensource.org/licenses/MIT
# ----------------------------------------------------------------------

# Unit tests for I/O operator templates

# OStreamOperator tests
add_executable(OStreamOperator_ut OStreamOperator_ut.cpp)
target_include_directories(OStreamOperator_ut
    PRIVATE
        ${CMAKE_SOURCE_DIR})
target_link_libraries(OStreamOperator_ut
    PRIVATE
        atlas_lib
        generation
        doctest::doctest)
add_test(NAME OStreamOperatorTests COMMAND OStreamOperator_ut)
set_target_properties(OStreamOperator_ut PROPERTIES
    RUNTIME_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/bin)

# IStreamOperator tests
add_executable(IStreamOperator_ut IStreamOperator_ut.cpp)
target_include_directories(IStreamOperator_ut
    PRIVATE
        ${CMAKE_SOURCE_DIR})
target_link_libraries(IStreamOperator_ut
    PRIVATE
        atlas_lib
        generation
        doctest::doctest)
add_test(NAME IStreamOperatorTests COMMAND IStreamOperator_ut)
set_target_properties(IStreamOperator_ut PROPERTIES
    RUNTIME_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/bin)
