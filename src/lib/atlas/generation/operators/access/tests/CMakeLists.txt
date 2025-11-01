# ----------------------------------------------------------------------
# Copyright 2025 Jody Hagins
# Distributed under the MIT Software License
# See accompanying file LICENSE or copy at
# https://opensource.org/licenses/MIT
# ----------------------------------------------------------------------

# Unit tests for access operator templates

# ArrowOperator tests
add_executable(ArrowOperator_ut ArrowOperator_ut.cpp)
target_include_directories(ArrowOperator_ut
    PRIVATE
        ${CMAKE_SOURCE_DIR})
target_link_libraries(ArrowOperator_ut
    PRIVATE
        atlas_lib
        generation
        doctest::doctest)
add_test(NAME ArrowOperatorTests COMMAND ArrowOperator_ut)
set_target_properties(ArrowOperator_ut PROPERTIES
    RUNTIME_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/bin)

# IndirectionOperator tests
add_executable(IndirectionOperator_ut IndirectionOperator_ut.cpp)
target_include_directories(IndirectionOperator_ut
    PRIVATE
        ${CMAKE_SOURCE_DIR})
target_link_libraries(IndirectionOperator_ut
    PRIVATE
        atlas_lib
        generation
        doctest::doctest)
add_test(NAME IndirectionOperatorTests COMMAND IndirectionOperator_ut)
set_target_properties(IndirectionOperator_ut PROPERTIES
    RUNTIME_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/bin)
