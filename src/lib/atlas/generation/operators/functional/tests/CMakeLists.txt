# ----------------------------------------------------------------------
# Copyright 2025 Jody Hagins
# Distributed under the MIT Software License
# See accompanying file LICENSE or copy at
# https://opensource.org/licenses/MIT
# ----------------------------------------------------------------------

# Unit tests for functional operator templates

# NullaryOperator tests
add_executable(NullaryOperator_ut NullaryOperator_ut.cpp)
target_include_directories(NullaryOperator_ut
    PRIVATE
        ${CMAKE_SOURCE_DIR})
target_link_libraries(NullaryOperator_ut
    PRIVATE
        atlas_lib
        generation
        doctest::doctest)
add_test(NAME NullaryOperatorTests COMMAND NullaryOperator_ut)
set_target_properties(NullaryOperator_ut PROPERTIES
    RUNTIME_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/bin)

# CallableOperator tests
add_executable(CallableOperator_ut CallableOperator_ut.cpp)
target_include_directories(CallableOperator_ut
    PRIVATE
        ${CMAKE_SOURCE_DIR})
target_link_libraries(CallableOperator_ut
    PRIVATE
        atlas_lib
        generation
        doctest::doctest)
add_test(NAME CallableOperatorTests COMMAND CallableOperator_ut)
set_target_properties(CallableOperator_ut PROPERTIES
    RUNTIME_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/bin)

# SubscriptOperator tests
add_executable(SubscriptOperator_ut SubscriptOperator_ut.cpp)
target_include_directories(SubscriptOperator_ut
    PRIVATE
        ${CMAKE_SOURCE_DIR})
target_link_libraries(SubscriptOperator_ut
    PRIVATE
        atlas_lib
        generation
        doctest::doctest)
add_test(NAME SubscriptOperatorTests COMMAND SubscriptOperator_ut)
set_target_properties(SubscriptOperator_ut PROPERTIES
    RUNTIME_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/bin)

# AddressOfOperator tests
add_executable(AddressOfOperator_ut AddressOfOperator_ut.cpp)
target_include_directories(AddressOfOperator_ut
    PRIVATE
        ${CMAKE_SOURCE_DIR})
target_link_libraries(AddressOfOperator_ut
    PRIVATE
        atlas_lib
        generation
        doctest::doctest)
add_test(NAME AddressOfOperatorTests COMMAND AddressOfOperator_ut)
set_target_properties(AddressOfOperator_ut PROPERTIES
    RUNTIME_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/bin)
