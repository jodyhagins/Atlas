# ----------------------------------------------------------------------
# Copyright 2025 Jody Hagins
# Distributed under the MIT Software License
# See accompanying file LICENSE or copy at
# https://opensource.org/licenses/MIT
# ----------------------------------------------------------------------

# Unit tests for comparison operator templates

# RelationalOperator tests
add_executable(RelationalOperator_ut RelationalOperator_ut.cpp)
target_include_directories(RelationalOperator_ut
    PRIVATE
        ${CMAKE_SOURCE_DIR})
target_link_libraries(RelationalOperator_ut
    PRIVATE
        atlas_lib
        generation
        doctest::doctest)
add_test(NAME RelationalOperatorTests COMMAND RelationalOperator_ut)
set_target_properties(RelationalOperator_ut PROPERTIES
    RUNTIME_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/bin)

# SpaceshipOperator tests
add_executable(SpaceshipOperator_ut SpaceshipOperator_ut.cpp)
target_include_directories(SpaceshipOperator_ut
    PRIVATE
        ${CMAKE_SOURCE_DIR})
target_link_libraries(SpaceshipOperator_ut
    PRIVATE
        atlas_lib
        generation
        doctest::doctest)
add_test(NAME SpaceshipOperatorTests COMMAND SpaceshipOperator_ut)
set_target_properties(SpaceshipOperator_ut PROPERTIES
    RUNTIME_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/bin)

# DefaultedEqualityOperator tests
add_executable(DefaultedEqualityOperator_ut DefaultedEqualityOperator_ut.cpp)
target_include_directories(DefaultedEqualityOperator_ut
    PRIVATE
        ${CMAKE_SOURCE_DIR})
target_link_libraries(DefaultedEqualityOperator_ut
    PRIVATE
        atlas_lib
        generation
        doctest::doctest)
add_test(NAME DefaultedEqualityOperatorTests COMMAND DefaultedEqualityOperator_ut)
set_target_properties(DefaultedEqualityOperator_ut PROPERTIES
    RUNTIME_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/bin)
