# ----------------------------------------------------------------------
# Copyright 2025 Jody Hagins
# Distributed under the MIT Software License
# See accompanying file LICENSE or copy at
# https://opensource.org/licenses/MIT
# ----------------------------------------------------------------------

# Unit tests for arithmetic operator templates

# AdditionOperator tests
add_executable(AdditionOperator_ut AdditionOperator_ut.cpp)
target_include_directories(AdditionOperator_ut
    PRIVATE
        ${CMAKE_SOURCE_DIR})
target_link_libraries(AdditionOperator_ut
    PRIVATE
        atlas_lib
        generation
        doctest::doctest)
add_test(NAME AdditionOperatorTests COMMAND AdditionOperator_ut)
set_target_properties(AdditionOperator_ut PROPERTIES
    RUNTIME_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/bin)

# SubtractionOperator tests
add_executable(SubtractionOperator_ut SubtractionOperator_ut.cpp)
target_include_directories(SubtractionOperator_ut
    PRIVATE
        ${CMAKE_SOURCE_DIR})
target_link_libraries(SubtractionOperator_ut
    PRIVATE
        atlas_lib
        generation
        doctest::doctest)
add_test(NAME SubtractionOperatorTests COMMAND SubtractionOperator_ut)
set_target_properties(SubtractionOperator_ut PROPERTIES
    RUNTIME_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/bin)

# MultiplicationOperator tests
add_executable(MultiplicationOperator_ut MultiplicationOperator_ut.cpp)
target_include_directories(MultiplicationOperator_ut
    PRIVATE
        ${CMAKE_SOURCE_DIR})
target_link_libraries(MultiplicationOperator_ut
    PRIVATE
        atlas_lib
        generation
        doctest::doctest)
add_test(NAME MultiplicationOperatorTests COMMAND MultiplicationOperator_ut)
set_target_properties(MultiplicationOperator_ut PROPERTIES
    RUNTIME_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/bin)

# DivisionOperator tests
add_executable(DivisionOperator_ut DivisionOperator_ut.cpp)
target_include_directories(DivisionOperator_ut
    PRIVATE
        ${CMAKE_SOURCE_DIR})
target_link_libraries(DivisionOperator_ut
    PRIVATE
        atlas_lib
        generation
        doctest::doctest)
add_test(NAME DivisionOperatorTests COMMAND DivisionOperator_ut)
set_target_properties(DivisionOperator_ut PROPERTIES
    RUNTIME_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/bin)

# ModuloOperator tests
add_executable(ModuloOperator_ut ModuloOperator_ut.cpp)
target_include_directories(ModuloOperator_ut
    PRIVATE
        ${CMAKE_SOURCE_DIR})
target_link_libraries(ModuloOperator_ut
    PRIVATE
        atlas_lib
        generation
        doctest::doctest)
add_test(NAME ModuloOperatorTests COMMAND ModuloOperator_ut)
set_target_properties(ModuloOperator_ut PROPERTIES
    RUNTIME_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/bin)

# UnaryOperators tests
add_executable(UnaryOperators_ut UnaryOperators_ut.cpp)
target_include_directories(UnaryOperators_ut
    PRIVATE
        ${CMAKE_SOURCE_DIR})
target_link_libraries(UnaryOperators_ut
    PRIVATE
        atlas_lib
        generation
        doctest::doctest)
add_test(NAME UnaryOperatorsTests COMMAND UnaryOperators_ut)
set_target_properties(UnaryOperators_ut PROPERTIES
    RUNTIME_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/bin)

# IncrementOperators tests
add_executable(IncrementOperators_ut IncrementOperators_ut.cpp)
target_include_directories(IncrementOperators_ut
    PRIVATE
        ${CMAKE_SOURCE_DIR})
target_link_libraries(IncrementOperators_ut
    PRIVATE
        atlas_lib
        generation
        doctest::doctest)
add_test(NAME IncrementOperatorsTests COMMAND IncrementOperators_ut)
set_target_properties(IncrementOperators_ut PROPERTIES
    RUNTIME_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/bin)
