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


add_executable(atlas_command_line_ut
    atlas_command_line_ut.cpp
)



add_executable(interaction_generator_ut
    interaction_generator_ut.cpp
)

add_executable(golden_ut
    golden_ut.cpp
)

add_executable(property_ut
    property_ut.cpp
)

add_executable(compilation_ut
    compilation_ut.cpp
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

target_link_libraries(golden_ut
    PRIVATE
        atlas_lib
        doctest::doctest
)

target_include_directories(golden_ut
    PRIVATE
        ${CMAKE_CURRENT_SOURCE_DIR}
)

target_link_libraries(property_ut
    PRIVATE
        atlas_lib
        doctest::doctest
        rapidcheck
)

target_include_directories(property_ut
    PRIVATE
        ${CMAKE_CURRENT_SOURCE_DIR}
        ${rapidcheck_SOURCE_DIR}/extras/doctest/include
)

target_link_libraries(compilation_ut
    PRIVATE
        atlas_lib
        doctest::doctest
)

target_include_directories(compilation_ut
    PRIVATE
        ${CMAKE_CURRENT_SOURCE_DIR}
)


add_test(NAME AtlasCommandLineTest COMMAND atlas_command_line_ut)

# Use DocTest's built-in discovery (runs at build time, not configure time)
include(${doctest_SOURCE_DIR}/scripts/cmake/doctest.cmake)
doctest_discover_tests(strong_type_generator_ut TEST_PREFIX "Generator : ")
doctest_discover_tests(interaction_generator_ut TEST_PREFIX "Interaction Generator : ")
doctest_discover_tests(golden_ut TEST_PREFIX "Golden : "
    PROPERTIES ENVIRONMENT "SOURCE_DIR=${CMAKE_SOURCE_DIR}"
)
doctest_discover_tests(property_ut TEST_PREFIX "Property : ")
doctest_discover_tests(compilation_ut TEST_PREFIX "Compilation : "
    PROPERTIES TIMEOUT 120 LABELS "slow"
)
