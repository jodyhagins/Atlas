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
    GIT_TAG 6b7985bc2562a135964bc03d69a3ef7af12b9309 # order-by-rand
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

target_link_libraries(strong_type_generator_ut
    PRIVATE
        atlas_lib
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
)

target_include_directories(atlas_command_line_ut
    PRIVATE
        ${CMAKE_CURRENT_SOURCE_DIR}
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

# Add tests to CTest
add_test(NAME AtlasTest COMMAND strong_type_generator_ut)
add_test(NAME AtlasToolTest COMMAND atlas_tool_ut)
set_tests_properties(AtlasToolTest PROPERTIES ENVIRONMENT "BUILD_DIR=${CMAKE_BINARY_DIR}")
add_test(NAME AtlasCommandLineTest COMMAND atlas_command_line_ut)
add_test(NAME ErrorHandlingTest COMMAND error_handling_ut)
add_test(NAME IntegrationTest COMMAND integration_ut)

# Note: I'm not a cmake guru - all this test splitting was done by claude-code
# when I told it that the tests were taking too long to run. At first it aplit
# them all manually, but I told it that seemed error prone, and asked it to
# find a way that wouldn't skip tests if I added more, and it came up with
# this idea and implemented it. Seems to work, even though it seems overly
# verbose.
#
# Configure which test cases should be split into subcases for parallelism
# Test cases not listed here will run as a single test with all their subcases
set(SPLIT_INTO_SUBCASES
    "Hash Support"
    "Subscript Operator Support"
    "Default Value Support"
)

# Helper function to create a valid CTest name from a test/subcase name
function(make_test_name OUTPUT_VAR test_case subcase)
    string(REGEX REPLACE "[^a-zA-Z0-9]+" "_" test_name "${test_case}")
    if(subcase)
        string(REGEX REPLACE "[^a-zA-Z0-9]+" "_" subcase_name "${subcase}")
        set(${OUTPUT_VAR} "GeneratedCode_${test_name}_${subcase_name}" PARENT_SCOPE)
    else()
        set(${OUTPUT_VAR} "GeneratedCode_${test_name}" PARENT_SCOPE)
    endif()
endfunction()

# Auto-discover and register all test cases from generated_code_ut
if(EXISTS ${CMAKE_CURRENT_BINARY_DIR}/generated_code_ut)
    # Get list of all test cases
    execute_process(
        COMMAND ${CMAKE_CURRENT_BINARY_DIR}/generated_code_ut --list-test-cases
        OUTPUT_VARIABLE ALL_TEST_CASES
        OUTPUT_STRIP_TRAILING_WHITESPACE
    )

    # Parse test case names (they appear between "===" lines)
    string(REGEX MATCHALL "[^\n]+" LINES "${ALL_TEST_CASES}")
    set(IN_TEST_CASES FALSE)
    foreach(line ${LINES})
        if(line MATCHES "^=+$")
            if(IN_TEST_CASES)
                set(IN_TEST_CASES FALSE)
            else()
                set(IN_TEST_CASES TRUE)
            endif()
        elseif(IN_TEST_CASES AND NOT line MATCHES "^$")
            list(APPEND TEST_CASE_LIST "${line}")
        endif()
    endforeach()

    # Register tests
    foreach(test_case ${TEST_CASE_LIST})
        if(test_case IN_LIST SPLIT_INTO_SUBCASES)
            # Get subcases for this test case
            execute_process(
                COMMAND ${CMAKE_CURRENT_BINARY_DIR}/generated_code_ut "--test-case=${test_case}" --list-test-cases --no-run
                OUTPUT_VARIABLE SUBCASES_OUTPUT
                RESULT_VARIABLE SUBCASE_RESULT
            )

            # For split test cases, we need to discover subcases differently
            # Run the test to discover subcases from actual execution
            execute_process(
                COMMAND ${CMAKE_CURRENT_BINARY_DIR}/generated_code_ut "--test-case=${test_case}" --no-run --duration=true
                OUTPUT_VARIABLE TEST_OUTPUT
            )

            # Extract subcase names from the source file
            file(STRINGS "${CMAKE_CURRENT_SOURCE_DIR}/generated_code_ut.cpp" SOURCE_LINES)
            set(IN_TEST_CASE FALSE)
            set(SUBCASE_LIST "")
            foreach(source_line ${SOURCE_LINES})
                if(source_line MATCHES "TEST_CASE\\(\"${test_case}\"\\)")
                    set(IN_TEST_CASE TRUE)
                elseif(IN_TEST_CASE AND source_line MATCHES "TEST_CASE\\(")
                    set(IN_TEST_CASE FALSE)
                elseif(IN_TEST_CASE AND source_line MATCHES "SUBCASE\\(\"([^\"]+)\"\\)")
                    list(APPEND SUBCASE_LIST "${CMAKE_MATCH_1}")
                endif()
            endforeach()

            # Register each subcase as a separate test
            foreach(subcase ${SUBCASE_LIST})
                make_test_name(test_name "${test_case}" "${subcase}")
                add_test(NAME ${test_name} COMMAND generated_code_ut "--test-case=${test_case}" "--subcase=${subcase}")
            endforeach()
        else()
            # Register test case without subcase splitting
            make_test_name(test_name "${test_case}" "")
            add_test(NAME ${test_name} COMMAND generated_code_ut "--test-case=${test_case}")
        endif()
    endforeach()
else()
    message(WARNING "generated_code_ut not found - tests will be registered after first build")
endif()
