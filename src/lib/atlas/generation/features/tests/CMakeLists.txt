# ----------------------------------------------------------------------
# Copyright 2025 Jody Hagins
# Distributed under the MIT Software License
# See accompanying file LICENSE or copy at
# https://opensource.org/licenses/MIT
# ----------------------------------------------------------------------

# Unit tests for feature templates

# ConstantTemplate tests
add_executable(ConstantTemplate_ut ConstantTemplate_ut.cpp)
target_include_directories(ConstantTemplate_ut
    PRIVATE
        ${CMAKE_SOURCE_DIR})
target_link_libraries(ConstantTemplate_ut
    PRIVATE
        atlas_lib
        generation
        doctest::doctest)
add_test(NAME ConstantTemplateTests COMMAND ConstantTemplate_ut)
set_target_properties(ConstantTemplate_ut PROPERTIES
    RUNTIME_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/bin)

# TemplateAssignmentOperator tests
add_executable(TemplateAssignmentOperator_ut TemplateAssignmentOperator_ut.cpp)
target_include_directories(TemplateAssignmentOperator_ut
    PRIVATE
        ${CMAKE_SOURCE_DIR})
target_link_libraries(TemplateAssignmentOperator_ut
    PRIVATE
        atlas_lib
        generation
        doctest::doctest)
add_test(NAME TemplateAssignmentOperatorTests COMMAND TemplateAssignmentOperator_ut)
set_target_properties(TemplateAssignmentOperator_ut PROPERTIES
    RUNTIME_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/bin)

# IteratorSupportTemplate tests
add_executable(IteratorSupportTemplate_ut IteratorSupportTemplate_ut.cpp)
target_include_directories(IteratorSupportTemplate_ut
    PRIVATE
        ${CMAKE_SOURCE_DIR})
target_link_libraries(IteratorSupportTemplate_ut
    PRIVATE
        atlas_lib
        generation
        doctest::doctest)
add_test(NAME IteratorSupportTemplateTests COMMAND IteratorSupportTemplate_ut)
set_target_properties(IteratorSupportTemplate_ut PROPERTIES
    RUNTIME_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/bin)

# ForwardedMemfnTemplate tests
add_executable(ForwardedMemfnTemplate_ut ForwardedMemfnTemplate_ut.cpp)
target_include_directories(ForwardedMemfnTemplate_ut
    PRIVATE
        ${CMAKE_SOURCE_DIR})
target_link_libraries(ForwardedMemfnTemplate_ut
    PRIVATE
        atlas_lib
        generation
        doctest::doctest)
add_test(NAME ForwardedMemfnTemplateTests COMMAND ForwardedMemfnTemplate_ut)
set_target_properties(ForwardedMemfnTemplate_ut PROPERTIES
    RUNTIME_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/bin)
