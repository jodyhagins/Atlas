# ----------------------------------------------------------------------
# Copyright 2025 Jody Hagins
# Distributed under the MIT Software License
# See accompanying file LICENSE or copy at
# https://opensource.org/licenses/MIT
# ----------------------------------------------------------------------
# Tests for StrongTypeGenerator

message(STATUS "Processing third-party DocTest...")
fetchcontent_declare(DocTest
    GIT_REPOSITORY https://github.com/jodyhagins/doctest.git
    GIT_TAG dev
    SYSTEM)

fetchcontent_makeavailable(DocTest)

message(STATUS "Processing third-party RapidCheck...")
set(RC_ENABLE_DOCTEST ON)

fetchcontent_declare(rapidcheck
    GIT_REPOSITORY https://github.com/jodyhagins/rapidcheck.git
    GIT_TAG wjh-master
    SYSTEM)

fetchcontent_makeavailable(rapidcheck)


# Test utilities library
add_library(atlas_test_utils
    STATIC
        CodeStructureParser.cpp)

target_link_libraries(atlas_test_utils
    PUBLIC
        atlas_lib)

target_include_directories(atlas_test_utils
    PUBLIC
        ${CMAKE_CURRENT_SOURCE_DIR})

# Helper function to add atlas test executables
function(add_atlas_test)
    set(options USE_RAPIDCHECK USE_TEST_UTILS)
    set(oneValueArgs TARGET TEST_NAME)
    set(multiValueArgs TEST_PROPERTIES)
    cmake_parse_arguments(ARG "${options}" "${oneValueArgs}" "${multiValueArgs}" ${ARGN})

    # Create executable
    add_executable(${ARG_TARGET} ${ARG_TARGET}.cpp)

    # Link libraries
    set(LIBS atlas_lib doctest::doctest)
    if(ARG_USE_TEST_UTILS)
        list(PREPEND LIBS atlas_test_utils)
    endif()
    if(ARG_USE_RAPIDCHECK)
        list(APPEND LIBS rapidcheck)
    endif()
    target_link_libraries(${ARG_TARGET} PRIVATE ${LIBS})

    # Include directories
    set(INCLUDES ${CMAKE_CURRENT_SOURCE_DIR})
    if(ARG_USE_RAPIDCHECK)
        list(APPEND INCLUDES ${rapidcheck_SOURCE_DIR}/extras/doctest/include)
    endif()
    target_include_directories(${ARG_TARGET} PRIVATE ${INCLUDES})

    # Add test
    if(ARG_TEST_NAME)
        add_test(NAME ${ARG_TEST_NAME} COMMAND ${ARG_TARGET})
        if(ARG_TEST_PROPERTIES)
            set_tests_properties(${ARG_TEST_NAME} PROPERTIES ${ARG_TEST_PROPERTIES})
        endif()
    endif()
endfunction()

# Add test executables
add_atlas_test(TARGET strong_type_generator_ut TEST_NAME GeneratorTests USE_TEST_UTILS USE_RAPIDCHECK)
add_atlas_test(TARGET atlas_command_line_ut TEST_NAME AtlasCommandLineTest USE_RAPIDCHECK)
add_atlas_test(TARGET atlas_main_ut TEST_NAME AtlasMainTests)
add_atlas_test(TARGET interaction_generator_ut TEST_NAME InteractionGeneratorTests USE_RAPIDCHECK)
add_atlas_test(TARGET property_ut TEST_NAME PropertyTests USE_RAPIDCHECK)
add_atlas_test(TARGET error_ut TEST_NAME ErrorConditionTests)
add_atlas_test(TARGET template_system_ut TEST_NAME TemplateSystemTests)
add_atlas_test(TARGET type_tokenizer_ut TEST_NAME TypeTokenizerTests)
add_atlas_test(TARGET token_to_header_mapper_ut TEST_NAME TokenToHeaderMapperTests)

# Tests that need special properties
add_atlas_test(TARGET golden_ut TEST_NAME GoldenTests)
set_tests_properties(GoldenTests PROPERTIES ENVIRONMENT "SOURCE_DIR=${CMAKE_SOURCE_DIR}")

# Compilation tests - use discovery for parallel execution
add_atlas_test(TARGET compilation_ut)
include(${doctest_SOURCE_DIR}/scripts/cmake/doctest.cmake)
doctest_discover_tests(compilation_ut TEST_PREFIX "Compilation : "
    PROPERTIES COST 99)

# Atlas tool tests - tests the actual executable
add_executable(atlas_tool_ut atlas_tool_ut.cpp)

target_link_libraries(atlas_tool_ut
    PRIVATE
        doctest::doctest)

target_compile_definitions(atlas_tool_ut
    PRIVATE
        ATLAS_TOOL_PATH="$<TARGET_FILE:atlas>")

add_test(NAME AtlasToolTests COMMAND atlas_tool_ut)
# Ensure the atlas executable is built before running the test
add_dependencies(atlas_tool_ut atlas)

# Arithmetic modes runtime tests - comprehensive test of all arithmetic modes
# Generate types for arithmetic modes runtime test
set(ARITHMETIC_MODES_INPUT "${CMAKE_CURRENT_SOURCE_DIR}/arithmetic_modes_test_input.atlas")
set(ARITHMETIC_MODES_OUTPUT "${CMAKE_CURRENT_BINARY_DIR}/arithmetic_modes_test_types.hpp")

add_custom_command(
    OUTPUT ${ARITHMETIC_MODES_OUTPUT}
    COMMAND atlas --input=${ARITHMETIC_MODES_INPUT} --output=${ARITHMETIC_MODES_OUTPUT}
    DEPENDS atlas ${ARITHMETIC_MODES_INPUT}
    COMMENT "Generating arithmetic modes test types"
)

add_executable(arithmetic_modes_runtime_ut
    arithmetic_modes_runtime_ut.cpp
    ${ARITHMETIC_MODES_OUTPUT}
)

target_include_directories(arithmetic_modes_runtime_ut
    PRIVATE
        ${CMAKE_CURRENT_BINARY_DIR}
)

target_link_libraries(arithmetic_modes_runtime_ut
    PRIVATE
        doctest::doctest
)

add_test(NAME ArithmeticModesRuntimeTests COMMAND arithmetic_modes_runtime_ut)
# Ensure types are generated before building the test
add_dependencies(arithmetic_modes_runtime_ut atlas)

# ======================================================================
# CONSTRAINT INTEGRATION TESTS
# ======================================================================
set(INPUT "${CMAKE_CURRENT_SOURCE_DIR}/constraint_integration_test_types.atlas")
set(OUTPUT "${CMAKE_CURRENT_BINARY_DIR}/constraint_integration_types.hpp")
add_custom_command(
    OUTPUT ${OUTPUT}
    COMMAND atlas --input=${INPUT} --output=${OUTPUT}
    DEPENDS atlas ${INPUT}
    COMMENT "Generating constraint integration test types"
)
add_executable(constraint_integration_ut
    constraint_integration_ut.cpp
    ${OUTPUT}
)
target_link_libraries(constraint_integration_ut PRIVATE
    doctest::doctest
)
add_test(NAME ConstraintIntegrationTests
    COMMAND constraint_integration_ut
)
target_include_directories(constraint_integration_ut
    PRIVATE
        ${CMAKE_CURRENT_BINARY_DIR}
)

set(INPUT "${CMAKE_CURRENT_SOURCE_DIR}/constraint_integration2_test_types.atlas")
set(OUTPUT "${CMAKE_CURRENT_BINARY_DIR}/constraint_integration2_test_types.hpp")
add_custom_command(
    OUTPUT ${OUTPUT}
    COMMAND atlas --input=${INPUT} --output=${OUTPUT}
    DEPENDS atlas ${INPUT}
    COMMENT "Generating more constraint integration test types"
)
add_executable(constraint_integration2_ut
    constraint_integration2_ut.cpp
    ${OUTPUT}
)
target_link_libraries(constraint_integration2_ut PRIVATE
    doctest::doctest
)
add_test(NAME MoreConstraintIntegrationTests
    COMMAND constraint_integration2_ut
)
target_include_directories(constraint_integration2_ut
    PRIVATE
        ${CMAKE_CURRENT_BINARY_DIR}
)

set(OPTIONAL_INPUT "${CMAKE_CURRENT_SOURCE_DIR}/nilable_test_types.atlas")
set(OPTIONAL_OUTPUT "${CMAKE_CURRENT_BINARY_DIR}/nilable_test_types.hpp")
add_custom_command(
    OUTPUT ${OPTIONAL_OUTPUT}
    COMMAND atlas --input=${OPTIONAL_INPUT} --output=${OPTIONAL_OUTPUT}
    DEPENDS atlas ${OPTIONAL_INPUT}
    COMMENT "Generating atlas::Nilable test types"
)

add_executable(nilable_ut
    nilable_ut.cpp
    ${OPTIONAL_OUTPUT}
)
target_link_libraries(nilable_ut PRIVATE doctest::doctest)
target_include_directories(nilable_ut PRIVATE
    ${CMAKE_CURRENT_BINARY_DIR}
    ${CMAKE_CURRENT_SOURCE_DIR}/../src/lib/atlas
)
add_test(NAME NilableTests COMMAND nilable_ut)

# C++20-specific optional tests
set(OPTIONAL_CPP20_INPUT "${CMAKE_CURRENT_SOURCE_DIR}/nilable_cpp20_test_types.atlas")
set(OPTIONAL_CPP20_OUTPUT "${CMAKE_CURRENT_BINARY_DIR}/nilable_cpp20_test_types.hpp")
add_custom_command(
    OUTPUT ${OPTIONAL_CPP20_OUTPUT}
    COMMAND atlas --input=${OPTIONAL_CPP20_INPUT} --output=${OPTIONAL_CPP20_OUTPUT}
    DEPENDS atlas ${OPTIONAL_CPP20_INPUT}
    COMMENT "Generating atlas::Nilable C++20 test types"
)

add_executable(nilable_cpp20_ut
    nilable_cpp20_ut.cpp
    ${OPTIONAL_CPP20_OUTPUT}
)
target_link_libraries(nilable_cpp20_ut PRIVATE doctest::doctest)
target_include_directories(nilable_cpp20_ut PRIVATE
    ${CMAKE_CURRENT_BINARY_DIR}
    ${CMAKE_CURRENT_SOURCE_DIR}/../src/lib/atlas
)
add_test(NAME NilableCpp20Tests COMMAND nilable_cpp20_ut)

# atlas::to_underlying() tests
set(TO_UNDERLYING_INPUT "${CMAKE_CURRENT_SOURCE_DIR}/to_underlying_test_types.atlas")
set(TO_UNDERLYING_OUTPUT "${CMAKE_CURRENT_BINARY_DIR}/to_underlying_test_types.hpp")
add_custom_command(
    OUTPUT ${TO_UNDERLYING_OUTPUT}
    COMMAND atlas --input=${TO_UNDERLYING_INPUT} --output=${TO_UNDERLYING_OUTPUT}
    DEPENDS atlas ${TO_UNDERLYING_INPUT}
    COMMENT "Generating atlas::to_underlying test types"
)
add_executable(to_underlying_ut
    to_underlying_ut.cpp
    ${TO_UNDERLYING_OUTPUT}
)
target_link_libraries(to_underlying_ut PRIVATE doctest::doctest)
target_include_directories(to_underlying_ut PRIVATE
    ${CMAKE_CURRENT_BINARY_DIR}
    ${CMAKE_CURRENT_SOURCE_DIR}/../src/lib/atlas
)
add_test(NAME ToUnderlyingTests COMMAND to_underlying_ut)

