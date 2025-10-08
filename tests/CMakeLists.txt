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

# Tests that need special properties
add_atlas_test(TARGET golden_ut TEST_NAME GoldenTests)
set_tests_properties(GoldenTests PROPERTIES ENVIRONMENT "SOURCE_DIR=${CMAKE_SOURCE_DIR}")

# Compilation tests - use discovery for parallel execution
add_atlas_test(TARGET compilation_ut)
include(${doctest_SOURCE_DIR}/scripts/cmake/doctest.cmake)
doctest_discover_tests(compilation_ut TEST_PREFIX "Compilation : "
    PROPERTIES COST 99
)

# Atlas tool tests - tests the actual executable
add_executable(atlas_tool_ut atlas_tool_ut.cpp)
target_link_libraries(atlas_tool_ut PRIVATE doctest::doctest)
target_compile_definitions(atlas_tool_ut PRIVATE
    ATLAS_TOOL_PATH="$<TARGET_FILE:atlas>")
add_test(NAME AtlasToolTests COMMAND atlas_tool_ut)
# Ensure the atlas executable is built before running the test
add_dependencies(atlas_tool_ut atlas)
