# ----------------------------------------------------------------------
# Copyright 2025 Jody Hagins
# Distributed under the MIT Software License
# See accompanying file LICENSE or copy at
# https://opensource.org/licenses/MIT
# ----------------------------------------------------------------------
# Tests for StrongTypeGenerator

message(STATUS "Processing third-party DocTest...")
FetchContent_Declare(
    DocTest
    GIT_REPOSITORY https://github.com/jodyhagins/doctest.git
    GIT_TAG dev
    SYSTEM
)
FetchContent_MakeAvailable(DocTest)

message(STATUS "Processing third-party RapidCheck...")
set(RC_ENABLE_DOCTEST ON)
FetchContent_Declare(
    rapidcheck
    GIT_REPOSITORY https://github.com/jodyhagins/rapidcheck.git
    GIT_TAG wjh-master
    SYSTEM
)
FetchContent_MakeAvailable(rapidcheck)


# Test utilities library
add_library(atlas_test_utils STATIC
    CodeStructureParser.cpp
)

target_link_libraries(atlas_test_utils
    PUBLIC
        atlas_lib
)

target_include_directories(atlas_test_utils
    PUBLIC
        ${CMAKE_CURRENT_SOURCE_DIR}
)

# Add test executables
add_executable(strong_type_generator_ut
    strong_type_generator_ut.cpp
)

add_executable(generated_code_ut
    generated_code_ut.cpp
)

add_executable(atlas_tool_ut
    atlas_tool_ut.cpp
)

add_executable(atlas_command_line_ut
    atlas_command_line_ut.cpp
)

add_executable(error_handling_ut
    error_handling_ut.cpp
)

add_executable(integration_ut
    integration_ut.cpp
)

add_executable(interaction_generator_ut
    interaction_generator_ut.cpp
)

add_executable(interaction_compilation_ut
    interaction_compilation_ut.cpp
)

add_executable(structural_validation_demo_ut
    structural_validation_demo_ut.cpp
)


target_link_libraries(strong_type_generator_ut
    PRIVATE
        atlas_test_utils
        doctest::doctest
        rapidcheck
)

target_include_directories(strong_type_generator_ut
    PRIVATE
        ${CMAKE_CURRENT_SOURCE_DIR}
        ${rapidcheck_SOURCE_DIR}/extras/doctest/include
)

target_link_libraries(generated_code_ut
    PRIVATE
        atlas_lib
        doctest::doctest
        rapidcheck
)

target_include_directories(generated_code_ut
    PRIVATE
        ${CMAKE_CURRENT_SOURCE_DIR}
        ${rapidcheck_SOURCE_DIR}/extras/doctest/include
)

target_link_libraries(atlas_tool_ut
    PRIVATE
        atlas_test_utils
        doctest::doctest
)

target_include_directories(atlas_tool_ut
    PRIVATE
        ${CMAKE_CURRENT_SOURCE_DIR}
)

target_compile_options(atlas_tool_ut
    PRIVATE
        ${ATLAS_WARNING_FLAGS}
)

target_link_libraries(atlas_command_line_ut
    PRIVATE
        atlas_lib
        doctest::doctest
        rapidcheck
)

target_include_directories(atlas_command_line_ut
    PRIVATE
        ${CMAKE_CURRENT_SOURCE_DIR}
        ${rapidcheck_SOURCE_DIR}/extras/doctest/include
)

target_link_libraries(error_handling_ut
    PRIVATE
        atlas_lib
        doctest::doctest
)

target_include_directories(error_handling_ut
    PRIVATE
        ${CMAKE_CURRENT_SOURCE_DIR}
)

target_link_libraries(integration_ut
    PRIVATE
        atlas_lib
        doctest::doctest
)

target_include_directories(integration_ut
    PRIVATE
        ${CMAKE_CURRENT_SOURCE_DIR}
)

target_link_libraries(interaction_generator_ut
    PRIVATE
        atlas_lib
        doctest::doctest
        rapidcheck
)

target_include_directories(interaction_generator_ut
    PRIVATE
        ${CMAKE_CURRENT_SOURCE_DIR}
        ${rapidcheck_SOURCE_DIR}/extras/doctest/include
)

target_link_libraries(interaction_compilation_ut
    PRIVATE
        atlas_lib
        doctest::doctest
)

target_include_directories(interaction_compilation_ut
    PRIVATE
        ${CMAKE_CURRENT_SOURCE_DIR}
)

target_link_libraries(structural_validation_demo_ut
    PRIVATE
        atlas_test_utils
        doctest::doctest
)

target_include_directories(structural_validation_demo_ut
    PRIVATE
        ${CMAKE_CURRENT_SOURCE_DIR}
)


add_test(NAME AtlasCommandLineTest COMMAND atlas_command_line_ut)
add_test(NAME ErrorHandlingTest COMMAND error_handling_ut)
add_test(NAME IntegrationTest COMMAND integration_ut)

# Use DocTest's built-in discovery (runs at build time, not configure time)
include(${doctest_SOURCE_DIR}/scripts/cmake/doctest.cmake)
doctest_discover_tests(strong_type_generator_ut TEST_PREFIX "Generator : ")
doctest_discover_tests(generated_code_ut TEST_PREFIX "Generated Code : ")
doctest_discover_tests(atlas_tool_ut TEST_PREFIX "Atlas Tool : "
    PROPERTIES ENVIRONMENT "BUILD_DIR=${CMAKE_BINARY_DIR}"
)
doctest_discover_tests(interaction_generator_ut TEST_PREFIX "Interaction Generator : ")
doctest_discover_tests(interaction_compilation_ut TEST_PREFIX "Interaction Compilation : ")
doctest_discover_tests(structural_validation_demo_ut TEST_PREFIX "Structural Demo : ")
