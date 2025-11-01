# ----------------------------------------------------------------------
# Copyright 2025 Jody Hagins
# Distributed under the MIT Software License
# See accompanying file LICENSE or copy at
# https://opensource.org/licenses/MIT
# ----------------------------------------------------------------------

# Unit tests for logical operator templates

# LogicalNotOperator tests
add_executable(LogicalNotOperator_ut LogicalNotOperator_ut.cpp)
target_include_directories(LogicalNotOperator_ut
    PRIVATE
        ${CMAKE_SOURCE_DIR})
target_link_libraries(LogicalNotOperator_ut
    PRIVATE
        atlas_lib
        generation
        doctest::doctest)
add_test(NAME LogicalNotOperatorTests COMMAND LogicalNotOperator_ut)
set_target_properties(LogicalNotOperator_ut PROPERTIES
    RUNTIME_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/bin)

# LogicalAndOperator tests
add_executable(LogicalAndOperator_ut LogicalAndOperator_ut.cpp)
target_include_directories(LogicalAndOperator_ut
    PRIVATE
        ${CMAKE_SOURCE_DIR})
target_link_libraries(LogicalAndOperator_ut
    PRIVATE
        atlas_lib
        generation
        doctest::doctest)
add_test(NAME LogicalAndOperatorTests COMMAND LogicalAndOperator_ut)
set_target_properties(LogicalAndOperator_ut PROPERTIES
    RUNTIME_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/bin)

# LogicalOrOperator tests
add_executable(LogicalOrOperator_ut LogicalOrOperator_ut.cpp)
target_include_directories(LogicalOrOperator_ut
    PRIVATE
        ${CMAKE_SOURCE_DIR})
target_link_libraries(LogicalOrOperator_ut
    PRIVATE
        atlas_lib
        generation
        doctest::doctest)
add_test(NAME LogicalOrOperatorTests COMMAND LogicalOrOperator_ut)
set_target_properties(LogicalOrOperator_ut PROPERTIES
    RUNTIME_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/bin)
